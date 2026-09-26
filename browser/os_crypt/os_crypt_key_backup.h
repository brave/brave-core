/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_OS_CRYPT_OS_CRYPT_KEY_BACKUP_H_
#define BRAVE_BROWSER_OS_CRYPT_OS_CRYPT_KEY_BACKUP_H_

#include <string>

#include "base/feature.h"
#include "base/files/file_path.h"

class PrefRegistrySimple;
class PrefService;

namespace brave {

// The goal here is to keep a backup of the Brave key outside of `Local State`.
// Currently, `Local State` is the only place with the key. If the
// file gets corrupted for whatever reason (customer edited, process killed or
// computer turned off mid-write to the file, 3rd party modified, etc) the
// customer would lose the key. This key is important - it's used to encrypt
// and decrypt details like saved passwords, cookies, payment methods, and more.
// If the key is lost, folks will lose access to that information.
//
// Brave's key itself is "wrapped", meaning it's encrypted by the OS encryption.
// In order to get the real key, the OS encryption would need to decrypt or
// "unwrap" this value.
inline constexpr base::FilePath::CharType kOSCryptKeyBackupFileName[] =
    FILE_PATH_LITERAL("OSCrypt Key Backup");

// What the backup looks like relative to the key currently in `Local State`.
// Recorded in local state for a later change to act on; nothing reads it yet.
enum class OSCryptKeyBackupState {
  kUnknown = 0,
  // No backup existed, and one has now been written.
  kCreated = 1,
  // A backup exists and holds the key that is in use.
  kMatchesLiveKey = 2,
  // A backup exists and holds a different key. The backup is left untouched:
  // if the key in `Local State` changed, the older one is the one worth
  // keeping. Note this also fires the one time OSCrypt re-wraps the same key to
  // enable auditing, because the comparison is on the wrapped bytes.
  kDiffersFromLiveKey = 3,
};

// Writes the backup at `path` unless one is already there. Blocking.
// Exposed because it carries the rule worth testing: an existing backup is
// never replaced.
OSCryptKeyBackupState WriteOSCryptKeyBackupIfAbsent(const base::FilePath& path,
                                                    std::string encrypted_key,
                                                    std::string app_bound_key);

// What happened on the restore path. Recorded in `Local State` under
// `brave.os_crypt.key_restore_result`; nothing is returned to the caller.
enum class OSCryptKeyRestoreResult {
  // `Local State` already had a key. Nothing was read from disk.
  kNotAttempted = 0,
  // The key was missing and there is no backup to put back.
  kNoBackup = 1,
  // The key was missing and the backup has been put back.
  kRestored = 2,
  // The key was missing and a backup exists but could not be used.
  kBackupUnusable = 3,
};

// Guarding this feature so we can control w/ Griffin. Enabled by default.
// It's worth noting that the variations seed is also stored in `Local State`.
BASE_DECLARE_FEATURE(kBraveOSCryptKeyRestore);

// Puts the backed-up key back when `Local State` has lost it. Must run before
// OSCrypt initializes, since that is what reads the key and what mints a
// replacement when it finds none.
//
// Action is only taken when the key is missing (value is missing or file is
// missing). No action taken when there's a key but it's wrong (we don't have a
// way to tell). That situation would go down existing code path where new key
// is issued. Each key is checked on its own, since the DPAPI key and the
// app-bound key belong to separate providers that replace them independently.
//
// Blocking, but only touches the disk on the failure path: when a key is
// present this reads one in-memory pref and returns.
void MaybeRestoreOSCryptKey(const base::FilePath& user_data_dir,
                            PrefService* local_state);

void RegisterOSCryptKeyBackupLocalStatePrefs(PrefRegistrySimple* registry);

// Writes the backup if there isn't one, on a blocking background task. Never
// replaces an existing backup. Does nothing until a key exists to copy.
void BackUpOSCryptKey(const base::FilePath& user_data_dir,
                      PrefService* local_state);

}  // namespace brave

#endif  // BRAVE_BROWSER_OS_CRYPT_OS_CRYPT_KEY_BACKUP_H_
