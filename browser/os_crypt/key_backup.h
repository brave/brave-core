/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_OS_CRYPT_KEY_BACKUP_H_
#define BRAVE_BROWSER_OS_CRYPT_KEY_BACKUP_H_

#include <string>

#include "base/files/file_path.h"

class PrefRegistrySimple;
class PrefService;

namespace brave {

// The goal here is to keep a backup of the OSCrypt key outside of
// `Local State`. Currently, `Local State` has the only copy of the key. If the
// file gets corrupted for whatever reason (customer edited, process killed or
// computer turned off mid-write to the file, 3rd party modified, etc) the
// customer would lose the key. This key is important - it's used to encrypt
// and decrypt details like saved passwords, cookies, payment methods, and more.
// If the key is lost, folks will lose access to that information.
//
// The key itself is "wrapped", meaning it's encrypted by the OS encryption.
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

// What happened on the restore path.
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

// Puts the backed-up key back when `Local State` has lost it. Must run before
// OSCrypt initializes, since that is what reads the key and what mints a
// replacement when it finds none.
//
// Only ever acts when the key is *absent*. A key that is present but wrong
// cannot be told apart from a key the user legitimately has now, and replacing
// a live key would orphan everything encrypted since it arrived. Absence is the
// one case that is unambiguous, and it is the case `Local State` corruption
// produces.
//
// Blocking, but only touches the disk on the failure path: when a key is
// present this reads one in-memory pref and returns.
OSCryptKeyRestoreResult MaybeRestoreOSCryptKey(
    const base::FilePath& user_data_dir,
    PrefService* local_state);

void RegisterOSCryptKeyBackupLocalStatePrefs(PrefRegistrySimple* registry);

// Writes the backup if there isn't one, on a blocking background task. Never
// replaces an existing backup. Does nothing until a key exists to copy.
void BackUpOSCryptKey(const base::FilePath& user_data_dir,
                      PrefService* local_state);

}  // namespace brave

#endif  // BRAVE_BROWSER_OS_CRYPT_KEY_BACKUP_H_
