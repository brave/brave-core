/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/os_crypt/key_backup.h"

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
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave {

namespace {

constexpr char kBackupStatePrefName[] = "brave.os_crypt.key_backup_state";

// Defined privately in components/os_crypt/async/browser/os_crypt_win.cc and
// chrome/browser/os_crypt/app_bound_encryption_provider_win.h, and repeated
// here because neither exposes them.
constexpr char kEncryptedKeyPrefName[] = "os_crypt.encrypted_key";
constexpr char kAppBoundEncryptedKeyPrefName[] =
    "os_crypt.app_bound_encrypted_key";

constexpr char kVersionKey[] = "version";
constexpr char kCreatedKey[] = "created";
constexpr char kOsCryptKey[] = "os_crypt";
constexpr char kEncryptedKeyKey[] = "encrypted_key";
constexpr char kAppBoundEncryptedKeyKey[] = "app_bound_encrypted_key";

constexpr int kCurrentVersion = 1;

constexpr char kHistogramSuffix[] = "OSCryptKeyBackup";

// The wrapped key held in an existing backup, or nullopt if there is no usable
// backup to compare against.
std::optional<std::string> ReadBackedUpKey(const base::FilePath& path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents)) {
    return std::nullopt;
  }

  std::optional<base::DictValue> root =
      base::JSONReader::ReadDict(contents, base::JSON_PARSE_RFC);
  if (!root) {
    // A backup we cannot read protects nothing, so let it be replaced.
    return std::nullopt;
  }

  const std::optional<int> version = root->FindInt(kVersionKey);
  if (!version || *version > kCurrentVersion) {
    // Written by a newer build. Leave it alone rather than discarding a format
    // we do not understand.
    return std::string();
  }

  const base::DictValue* os_crypt = root->FindDict(kOsCryptKey);
  if (!os_crypt) {
    return std::nullopt;
  }
  const std::string* key = os_crypt->FindString(kEncryptedKeyKey);
  return key ? std::optional<std::string>(*key) : std::nullopt;
}

}  // namespace

OSCryptKeyBackupState WriteOSCryptKeyBackupIfAbsent(const base::FilePath& path,
                                                    std::string encrypted_key,
                                                    std::string app_bound_key) {
  if (const std::optional<std::string> existing = ReadBackedUpKey(path)) {
    // Never replace a backup. If the live key has changed, the one already
    // written is the one worth keeping.
    return *existing == encrypted_key
               ? OSCryptKeyBackupState::kMatchesLiveKey
               : OSCryptKeyBackupState::kDiffersFromLiveKey;
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

void RegisterOSCryptKeyBackupLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(
      kBackupStatePrefName, static_cast<int>(OSCryptKeyBackupState::kUnknown));
}

void BackUpOSCryptKey(const base::FilePath& user_data_dir,
                      PrefService* local_state) {
  if (user_data_dir.empty() || !local_state) {
    return;
  }

  // Read on the UI thread, where the prefs live. A key minted earlier this
  // session but not yet flushed is still visible here, which reading
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
                     local_state->GetString(kAppBoundEncryptedKeyPrefName)),
      // `local_state` is owned by BrowserProcessImpl and outlives every task
      // posted here; SKIP_ON_SHUTDOWN keeps this from running during teardown.
      base::BindOnce(&OnBackupFinished, base::Unretained(local_state)));
}

}  // namespace brave
