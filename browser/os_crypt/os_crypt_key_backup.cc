/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/os_crypt/os_crypt_key_backup.h"

#include <optional>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/json/values_util.h"
#include "base/location.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/os_crypt/app_bound_encryption_provider_win.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave {

namespace {

constexpr char kBackupStatePrefName[] = "brave.os_crypt.key_backup_state";
constexpr char kRestoreResultPrefName[] = "brave.os_crypt.key_restore_result";

// Defined privately in components/os_crypt/async/browser/os_crypt_win.cc, so
// repeated here. The app-bound equivalent is public, and is used directly as
// `os_crypt_async::kAppBoundEncryptedKeyPrefName`.
constexpr char kEncryptedKeyPrefName[] = "os_crypt.encrypted_key";

constexpr char kVersionKey[] = "version";
constexpr char kCreatedKey[] = "created";
constexpr char kOsCryptKey[] = "os_crypt";
constexpr char kEncryptedKeyKey[] = "encrypted_key";
constexpr char kAppBoundEncryptedKeyKey[] = "app_bound_encrypted_key";

constexpr int kCurrentVersion = 1;

constexpr char kHistogramSuffix[] = "OSCryptKeyBackup";

enum class BackupReadResult {
  kOk,
  // Nothing on disk yet.
  kAbsent,
  // Present but not usable, so it protects nothing and may be replaced.
  kUnreadable,
  // Written by a newer build. Not understood, but not ours to discard either.
  kNewerVersion,
};

struct Backup {
  BackupReadResult result = BackupReadResult::kAbsent;
  std::string encrypted_key;
  std::string app_bound_key;
};

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

  const std::optional<int> version = root->FindInt(kVersionKey);
  if (!version || *version > kCurrentVersion) {
    backup.result = BackupReadResult::kNewerVersion;
    return backup;
  }

  const base::DictValue* os_crypt = root->FindDict(kOsCryptKey);
  const std::string* key =
      os_crypt ? os_crypt->FindString(kEncryptedKeyKey) : nullptr;
  if (!key || key->empty()) {
    backup.result = BackupReadResult::kUnreadable;
    return backup;
  }

  backup.result = BackupReadResult::kOk;
  backup.encrypted_key = *key;
  if (const std::string* app_bound =
          os_crypt->FindString(kAppBoundEncryptedKeyKey)) {
    backup.app_bound_key = *app_bound;
  }
  return backup;
}

}  // namespace

OSCryptKeyBackupState WriteOSCryptKeyBackupIfAbsent(const base::FilePath& path,
                                                    std::string encrypted_key,
                                                    std::string app_bound_key) {
  const Backup existing = ReadBackup(path);
  switch (existing.result) {
    case BackupReadResult::kOk:
      // Never replace a backup. If the live key has changed, the one already
      // written is the one worth keeping.
      return existing.encrypted_key == encrypted_key
                 ? OSCryptKeyBackupState::kMatchesLiveKey
                 : OSCryptKeyBackupState::kDiffersFromLiveKey;
    case BackupReadResult::kNewerVersion:
      return OSCryptKeyBackupState::kDiffersFromLiveKey;
    case BackupReadResult::kAbsent:
    case BackupReadResult::kUnreadable:
      break;
  }

  base::DictValue os_crypt;
  os_crypt.Set(kEncryptedKeyKey, std::move(encrypted_key));
  if (!app_bound_key.empty()) {
    os_crypt.Set(kAppBoundEncryptedKeyKey, std::move(app_bound_key));
  }

  base::DictValue root;
  root.Set(kVersionKey, kCurrentVersion);
  root.Set(kCreatedKey, base::TimeToValue(base::Time::Now()));
  root.Set(kOsCryptKey, std::move(os_crypt));

  std::string json;
  if (!base::JSONWriter::WriteWithOptions(
          root, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json)) {
    return OSCryptKeyBackupState::kUnknown;
  }

  if (!base::ImportantFileWriter::WriteFileAtomically(path, json,
                                                      kHistogramSuffix)) {
    return OSCryptKeyBackupState::kUnknown;
  }

  return OSCryptKeyBackupState::kCreated;
}

namespace {

void OnBackupFinished(PrefService* local_state, OSCryptKeyBackupState state) {
  local_state->SetInteger(kBackupStatePrefName, static_cast<int>(state));
}

}  // namespace

OSCryptKeyRestoreResult MaybeRestoreOSCryptKey(
    const base::FilePath& user_data_dir,
    PrefService* local_state) {
  if (user_data_dir.empty() || !local_state) {
    return OSCryptKeyRestoreResult::kNotAttempted;
  }

  // This is the case hit most of the time.
  if (!local_state->GetString(kEncryptedKeyPrefName).empty()) {
    return OSCryptKeyRestoreResult::kNotAttempted;
  }

  // Simple lambda for recording how the restore went in `Local State`.
  // This is something we'd have to ask folks to examine manually - we don't
  // show the result anywhere.
  auto record = [local_state](OSCryptKeyRestoreResult result) {
    local_state->SetInteger(kRestoreResultPrefName, static_cast<int>(result));
    return result;
  };

  const Backup backup =
      ReadBackup(user_data_dir.Append(kOSCryptKeyBackupFileName));
  switch (backup.result) {
    case BackupReadResult::kAbsent:
      return record(OSCryptKeyRestoreResult::kNoBackup);
    case BackupReadResult::kUnreadable:
    case BackupReadResult::kNewerVersion:
      return record(OSCryptKeyRestoreResult::kBackupUnusable);
    case BackupReadResult::kOk:
      break;
  }

  // Whether the restored key still unwraps is not checked here. If DPAPI can no
  // longer unwrap the Brave key, OSCrypt will fail to decrypt it and we'll
  // create a new Brave key. This extra step at least gives us a chance to try
  // the key before going down that road.
  local_state->SetString(kEncryptedKeyPrefName, backup.encrypted_key);
  if (!backup.app_bound_key.empty()) {
    local_state->SetString(os_crypt_async::kAppBoundEncryptedKeyPrefName,
                           backup.app_bound_key);
  }
  return record(OSCryptKeyRestoreResult::kRestored);
}

void RegisterOSCryptKeyBackupLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(
      kBackupStatePrefName, static_cast<int>(OSCryptKeyBackupState::kUnknown));
  registry->RegisterIntegerPref(
      kRestoreResultPrefName,
      static_cast<int>(OSCryptKeyRestoreResult::kNotAttempted));
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

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&WriteOSCryptKeyBackupIfAbsent,
                     user_data_dir.Append(kOSCryptKeyBackupFileName),
                     std::move(encrypted_key),
                     local_state->GetString(
                         os_crypt_async::kAppBoundEncryptedKeyPrefName)),
      // `local_state` is owned by BrowserProcessImpl and outlives every task
      // posted here; SKIP_ON_SHUTDOWN keeps this from running during teardown.
      base::BindOnce(&OnBackupFinished, base::Unretained(local_state)));
}

}  // namespace brave
