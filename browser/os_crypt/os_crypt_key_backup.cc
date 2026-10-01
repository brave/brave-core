/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/os_crypt/os_crypt_key_backup.h"

#include <optional>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/containers/span.h"
#include "base/feature_list.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/json/values_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/browser/os_crypt/os_crypt_key_backup_com_bridge.h"
#include "chrome/browser/os_crypt/app_bound_encryption_provider_win.h"
#include "components/prefs/pref_service.h"

namespace brave {

namespace {

// Defined privately in components/os_crypt/async/browser/os_crypt_win.cc, so
// repeated here. The app-bound equivalent is public, and is used directly as
// `os_crypt_async::kAppBoundEncryptedKeyPrefName`.
constexpr char kEncryptedKeyPrefName[] = "os_crypt.encrypted_key";

constexpr char kOsCryptKey[] = "os_crypt";
constexpr char kEncryptedKeyHistoryKey[] = "encrypted_key_history";
constexpr char kAppBoundEncryptedKeyHistoryKey[] =
    "app_bound_encrypted_key_history";
constexpr char kWrappedKeyKey[] = "wrapped_key";
constexpr char kCreatedKey[] = "created";

constexpr char kHistogramSuffix[] = "OSCryptKeyBackup";
constexpr char kRestoreHistogramSuffix[] = "OSCryptKeyRestore";

// Restore log schema: a single, ever-growing list of records, one per
// restore attempt. Unlike the backup file's history, this is never
// trimmed - restores are rare (only happen when Local State has lost the
// key), so the file stays small in practice, and the entire point is to
// never lose a record of one having happened.
constexpr char kRestoresKey[] = "os_crypt_restores";
constexpr char kDpapiResultKey[] = "dpapi_result";
constexpr char kAppBoundResultKey[] = "app_bound_result";

// Keep at most this many distinct unwrapped keys per key type. Old enough
// that the underlying key has rotated this many times without a restore ever
// happening is considered not worth protecting against.
constexpr size_t kMaxHistoryEntries = 3;

// Unwraps a wrapped key (a base64 string exactly as stored in a Local State
// pref or in this backup file) into raw bytes, or nullopt if it can't be
// unwrapped. Bound to `UnwrapWithDPAPI` for the DPAPI key, or to
// `BlockingUnwrapAppBoundKeyOnCOMSTA` (with a step description) for the
// app-bound key.
using UnwrapCallback =
    base::RepeatingCallback<std::optional<std::vector<uint8_t>>(
        base::span<const uint8_t>)>;

// index 0 = oldest, last index = newest. This matches natural chronological
// reading order in the raw JSON file.
struct HistoryEntry {
  std::string wrapped_key;
  base::Time created;
};
using History = std::vector<HistoryEntry>;

enum class BackupReadResult {
  kOk,
  // Nothing on disk yet.
  kAbsent,
  // Present but not usable, so it protects nothing and may be replaced.
  kUnreadable,
};

struct Backup {
  BackupReadResult result = BackupReadResult::kAbsent;
  History encrypted_key_history;
  History app_bound_key_history;
};

// Parses one history list from `dict`, returning an empty history if
// `list_key` is absent (a valid state - e.g. app-bound is never present on
// non-system-level installs). Malformed entries are dropped rather than
// failing the whole read, since a single bad entry shouldn't discard the
// rest of an otherwise-usable history.
//
// Keeps at most the newest `kMaxHistoryEntries` regardless of how many are
// on disk - the cap is enforced on write, but a hand-edited or otherwise
// corrupted file could contain more, and restore's per-entry verification
// (a blocking elevation-service round trip, for the app-bound history) must
// not scale with an attacker- or corruption-controlled entry count.
History ParseHistory(const base::DictValue& dict, std::string_view list_key) {
  History history;
  const base::ListValue* list = dict.FindList(list_key);
  if (!list) {
    return history;
  }
  for (const base::Value& entry : *list) {
    const base::DictValue* entry_dict = entry.GetIfDict();
    if (!entry_dict) {
      continue;
    }
    const std::string* wrapped_key = entry_dict->FindString(kWrappedKeyKey);
    std::optional<base::Time> created =
        base::ValueToTime(entry_dict->Find(kCreatedKey));
    if (!wrapped_key || wrapped_key->empty() || !created) {
      continue;
    }
    history.push_back({*wrapped_key, *created});
  }
  if (history.size() > kMaxHistoryEntries) {
    history.erase(history.begin(),
                  history.begin() + (history.size() - kMaxHistoryEntries));
  }
  return history;
}

base::ListValue HistoryToValue(const History& history) {
  base::ListValue list;
  for (const HistoryEntry& entry : history) {
    base::DictValue entry_dict;
    entry_dict.Set(kWrappedKeyKey, entry.wrapped_key);
    entry_dict.Set(kCreatedKey, base::TimeToValue(entry.created));
    list.Append(std::move(entry_dict));
  }
  return list;
}

Backup ReadBackup(const base::FilePath& path) {
  Backup backup;

  std::string contents;
  if (!base::ReadFileToString(path, &contents)) {
    backup.result = BackupReadResult::kAbsent;
    return backup;
  }

  std::optional<base::DictValue> root =
      base::JSONReader::ReadDict(contents, base::JSON_PARSE_RFC);
  if (!root) {
    backup.result = BackupReadResult::kUnreadable;
    return backup;
  }

  const base::DictValue* os_crypt = root->FindDict(kOsCryptKey);
  if (!os_crypt) {
    backup.result = BackupReadResult::kUnreadable;
    return backup;
  }

  backup.result = BackupReadResult::kOk;
  backup.encrypted_key_history =
      ParseHistory(*os_crypt, kEncryptedKeyHistoryKey);
  backup.app_bound_key_history =
      ParseHistory(*os_crypt, kAppBoundEncryptedKeyHistoryKey);
  return backup;
}

bool WriteBackup(const base::FilePath& path, const Backup& backup) {
  base::DictValue os_crypt;
  os_crypt.Set(kEncryptedKeyHistoryKey,
               HistoryToValue(backup.encrypted_key_history));
  // Omit the app-bound list entirely when empty, rather than writing an
  // empty list, so a reader can tell "never had one" apart from "had one,
  // now empty" (the latter shouldn't occur, but this is the cheapest way to
  // preserve that distinction).
  if (!backup.app_bound_key_history.empty()) {
    os_crypt.Set(kAppBoundEncryptedKeyHistoryKey,
                 HistoryToValue(backup.app_bound_key_history));
  }

  base::DictValue root;
  root.Set(kOsCryptKey, std::move(os_crypt));

  const std::optional<std::string> json =
      base::WriteJsonWithOptions(root, base::OPTIONS_PRETTY_PRINT);
  if (!json) {
    return false;
  }

  return base::ImportantFileWriter::WriteFileAtomically(path, *json,
                                                        kHistogramSuffix);
}

// Appends one record to the restore log at `path`: a timestamp plus the
// per-key outcomes of this attempt (`dpapi_result`/`app_bound_result`, either
// of which may be `kNotAttempted`). Reads whatever is already there first, so
// a restore is never lost even though this file is otherwise never touched
// except to append - unlike the backup file, nothing here is ever trimmed or
// replaced.
void AppendRestoreRecord(const base::FilePath& path,
                         OSCryptKeyRestoreResult dpapi_result,
                         OSCryptKeyRestoreResult app_bound_result) {
  std::string contents;
  std::optional<base::DictValue> root;
  if (base::ReadFileToString(path, &contents)) {
    root = base::JSONReader::ReadDict(contents, base::JSON_PARSE_RFC);
  }
  if (!root) {
    root.emplace();
  }

  base::ListValue* restores = root->FindList(kRestoresKey);
  base::ListValue new_restores;
  if (restores) {
    new_restores = std::move(*restores);
  }

  base::DictValue record;
  record.Set(kCreatedKey, base::TimeToValue(base::Time::Now()));
  record.Set(kDpapiResultKey, static_cast<int>(dpapi_result));
  record.Set(kAppBoundResultKey, static_cast<int>(app_bound_result));
  new_restores.Append(std::move(record));
  root->Set(kRestoresKey, std::move(new_restores));

  const std::optional<std::string> json =
      base::WriteJsonWithOptions(*root, base::OPTIONS_PRETTY_PRINT);
  if (!json) {
    return;
  }
  base::ImportantFileWriter::WriteFileAtomically(path, *json,
                                                 kRestoreHistogramSuffix);
}

// Converts a wrapped key as stored (base64) into raw bytes for `unwrap`, or
// nullopt if the string isn't valid base64.
std::optional<std::vector<uint8_t>> UnwrapBase64(
    const UnwrapCallback& unwrap,
    const std::string& wrapped_key_base64) {
  std::optional<std::vector<uint8_t>> raw =
      base::Base64Decode(wrapped_key_base64);
  if (!raw) {
    return std::nullopt;
  }
  return unwrap.Run(*raw);
}

// If `live_wrapped_key` unwraps and matches (by unwrapped value) any entry
// already in `history`, does nothing and returns false. Otherwise (including
// when the live key doesn't unwrap at all - nothing to compare, so nothing
// to add) appends {`live_wrapped_key`, now} to `history`, evicting the
// oldest entry if `history.size()` would exceed `kMaxHistoryEntries`, and
// returns true.
//
// Scans `history` newest-to-oldest: the common case (key hasn't rotated)
// matches the newest entry and returns after a single comparison. Only a
// genuine rotation walks further back to confirm the live key is truly new
// - the same rare event that's about to trigger an append anyway.
bool AppendIfNewDistinctKey(History& history,
                            const std::string& live_wrapped_key,
                            const UnwrapCallback& unwrap) {
  std::optional<std::vector<uint8_t>> live_unwrapped =
      UnwrapBase64(unwrap, live_wrapped_key);
  if (!live_unwrapped) {
    return false;
  }

  // Newest first: the common case (key hasn't rotated) matches on the first
  // iteration.
  for (const HistoryEntry& entry : std::views::reverse(history)) {
    std::optional<std::vector<uint8_t>> entry_unwrapped =
        UnwrapBase64(unwrap, entry.wrapped_key);
    if (entry_unwrapped == live_unwrapped) {
      return false;
    }
  }

  if (history.size() >= kMaxHistoryEntries) {
    history.erase(history.begin());
  }
  history.push_back({live_wrapped_key, base::Time::Now()});
  return true;
}

enum class HistoryRestoreOutcome {
  kNoHistory,
  kVerified,
  kUnverifiedFallback,
};

struct HistoryRestoreResult {
  HistoryRestoreOutcome outcome;
  // Valid only when outcome != kNoHistory.
  std::string wrapped_key;
};

// Walks `history` newest-to-oldest, unwrapping each entry, and returns the
// first one that verifies. If none verify, returns the newest entry anyway
// (OSCrypt's own new-key-minting fallback is the safety net downstream if it
// doesn't work either).
HistoryRestoreResult RestoreFromHistory(const History& history,
                                        std::string_view key_name_for_logging,
                                        const UnwrapCallback& unwrap) {
  if (history.empty()) {
    return {HistoryRestoreOutcome::kNoHistory, std::string()};
  }

  size_t attempt = 0;
  // Newest first: verify the freshest key before falling back to older ones.
  for (const HistoryEntry& entry : std::views::reverse(history)) {
    ++attempt;
    VLOG(1) << "OSCrypt key backup: verifying " << key_name_for_logging
            << " backup entry " << attempt << " of " << history.size();
    if (UnwrapBase64(unwrap, entry.wrapped_key)) {
      return {HistoryRestoreOutcome::kVerified, entry.wrapped_key};
    }
  }

  return {HistoryRestoreOutcome::kUnverifiedFallback,
          history.back().wrapped_key};
}

UnwrapCallback DPAPIUnwrapCallback() {
  return base::BindRepeating(&UnwrapWithDPAPI);
}

// `step_description` is the callback's fixed configuration and `wrapped_key`
// is supplied per invocation, so this can't be a plain `BindRepeating` of
// `BlockingUnwrapAppBoundKeyOnCOMSTA` - `Bind` only binds a prefix of a
// function's parameters, and `step_description` is the function's second
// parameter, not its first.
UnwrapCallback AppBoundUnwrapCallback(std::string_view step_description) {
  return base::BindRepeating(
      [](std::string step_description, base::span<const uint8_t> wrapped_key) {
        return BlockingUnwrapAppBoundKeyOnCOMSTA(wrapped_key, step_description);
      },
      std::string(step_description));
}

}  // namespace

OSCryptKeyBackupResult AppendOSCryptKeyBackupIfNew(const base::FilePath& path,
                                                   std::string encrypted_key,
                                                   std::string app_bound_key) {
  Backup backup = ReadBackup(path);
  bool changed = false;

  if (!encrypted_key.empty()) {
    changed |= AppendIfNewDistinctKey(backup.encrypted_key_history,
                                      encrypted_key, DPAPIUnwrapCallback());
  }

  if (!app_bound_key.empty()) {
    changed |= AppendIfNewDistinctKey(
        backup.app_bound_key_history, app_bound_key,
        AppBoundUnwrapCallback("appending app-bound backup entry"));
  }

  if (!changed) {
    return OSCryptKeyBackupResult::kUpToDate;
  }

  // Reaching here is uncommon. The key only changes when DPAPI or app-bound
  // re-wraps it (e.g. after a Windows credential change), which is rare. Worth
  // recording in case there is a problem to help the customer narrow down when
  // it happened.

  if (!WriteBackup(path, backup)) {
    LOG(ERROR) << "OSCrypt key backup: failed to write " << path;
    return OSCryptKeyBackupResult::kWriteFailed;
  }

  LOG(WARNING) << "OSCrypt key backup: appended a new history entry for "
               << path;
  return OSCryptKeyBackupResult::kAppended;
}

// Guarding this feature so we can control w/ Griffin. Enabled by default.
// It's worth noting that the variations seed is also stored in `Local State`.
BASE_FEATURE(kBraveOSCryptKeyRestore, base::FEATURE_ENABLED_BY_DEFAULT);

void MaybeRestoreOSCryptKey(const base::FilePath& user_data_dir,
                            PrefService* local_state) {
  if (!base::FeatureList::IsEnabled(kBraveOSCryptKeyRestore)) {
    return;
  }

  if (user_data_dir.empty() || !local_state) {
    return;
  }

  // Exit out if the key is present. This is the case hit most of the time.
  if (!local_state->GetString(kEncryptedKeyPrefName).empty()) {
    return;
  }

  // If we get here, the DPAPI key is missing and we're going to attempt a
  // a restore. This includes appending a record for the restore log.
  //
  // The result for the restore is stored for both the DPAPI key and (if
  // applicable) the app-bound key. The app-bound key is only used for
  // system level installs.
  OSCryptKeyRestoreResult dpapi_outcome =
      OSCryptKeyRestoreResult::kNotAttempted;
  OSCryptKeyRestoreResult app_bound_outcome =
      OSCryptKeyRestoreResult::kNotAttempted;

  const Backup backup =
      ReadBackup(user_data_dir.Append(kOSCryptKeyBackupFileName));
  switch (backup.result) {
    case BackupReadResult::kAbsent:
      dpapi_outcome = OSCryptKeyRestoreResult::kNoBackup;
      AppendRestoreRecord(user_data_dir.Append(kOSCryptKeyRestoreFileName),
                          dpapi_outcome, app_bound_outcome);
      return;
    case BackupReadResult::kUnreadable:
      dpapi_outcome = OSCryptKeyRestoreResult::kBackupUnusable;
      AppendRestoreRecord(user_data_dir.Append(kOSCryptKeyRestoreFileName),
                          dpapi_outcome, app_bound_outcome);
      return;
    case BackupReadResult::kOk:
      break;
  }

  // Attempt to restore the DPAPI key.
  if (!backup.encrypted_key_history.empty()) {
    // Restore the DPAPI key. Whether the restored key still unwraps has
    // already been checked by `RestoreFromHistory` on a best-effort basis;
    // if nothing in history verified, OSCrypt will fail to decrypt the
    // unverified fallback and mint a replacement Brave key. This extra step
    // at least gives us a chance to try a key that's known to work before
    // going down that road.
    const HistoryRestoreResult dpapi_result = RestoreFromHistory(
        backup.encrypted_key_history, "DPAPI", DPAPIUnwrapCallback());
    local_state->SetString(kEncryptedKeyPrefName, dpapi_result.wrapped_key);
    dpapi_outcome = dpapi_result.outcome == HistoryRestoreOutcome::kVerified
                        ? OSCryptKeyRestoreResult::kRestoredVerified
                        : OSCryptKeyRestoreResult::kRestoredUnverifiedFallback;
  } else {
    dpapi_outcome = OSCryptKeyRestoreResult::kBackupUnusable;
  }

  // Possibly restore the app-bound key. This is used for system-level
  // installs and works with the elevation service to encrypt/decrypt.
  // While both keys are stored in `Local State` and realistically would be
  // lost at the same time if `Local State` became corrupt, we don't want to
  // assume and overwrite this if it has a value. `app_bound_key_history` is
  // only ever non-empty on system-level installs (the app-bound provider
  // never populates the live pref otherwise), so this branch - and
  // `app_bound_outcome` - naturally stays untouched elsewhere.
  if (local_state->GetString(os_crypt_async::kAppBoundEncryptedKeyPrefName)
          .empty() &&
      !backup.app_bound_key_history.empty()) {
    const HistoryRestoreResult app_bound_result = RestoreFromHistory(
        backup.app_bound_key_history, "app-bound",
        AppBoundUnwrapCallback("verifying app-bound backup entry"));
    local_state->SetString(os_crypt_async::kAppBoundEncryptedKeyPrefName,
                           app_bound_result.wrapped_key);
    app_bound_outcome =
        app_bound_result.outcome == HistoryRestoreOutcome::kVerified
            ? OSCryptKeyRestoreResult::kRestoredVerified
            : OSCryptKeyRestoreResult::kRestoredUnverifiedFallback;
  }

  AppendRestoreRecord(user_data_dir.Append(kOSCryptKeyRestoreFileName),
                      dpapi_outcome, app_bound_outcome);
}

void BackUpOSCryptKey(const base::FilePath& user_data_dir,
                      PrefService* local_state) {
  if (user_data_dir.empty() || !local_state) {
    return;
  }

  // Read on the UI thread, where the prefs live. A Brave key created  earlier
  // this session but not yet flushed is still visible here, which reading
  // `Local State` from disk would miss.
  std::string encrypted_key = local_state->GetString(kEncryptedKeyPrefName);
  if (encrypted_key.empty()) {
    // No key to copy yet. A later launch will find one.
    return;
  }

  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(base::IgnoreResult(&AppendOSCryptKeyBackupIfNew),
                     user_data_dir.Append(kOSCryptKeyBackupFileName),
                     std::move(encrypted_key),
                     local_state->GetString(
                         os_crypt_async::kAppBoundEncryptedKeyPrefName)));
}

}  // namespace brave
