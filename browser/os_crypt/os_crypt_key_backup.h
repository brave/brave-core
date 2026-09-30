/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_OS_CRYPT_OS_CRYPT_KEY_BACKUP_H_
#define BRAVE_BROWSER_OS_CRYPT_OS_CRYPT_KEY_BACKUP_H_

#include <string>

#include "base/feature.h"
#include "base/files/file_path.h"

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
// "unwrap" this value. DPAPI and app-bound encryption are both
// non-deterministic - wrapping the same underlying key twice produces
// different ciphertext bytes each time - so a history of distinct *unwrapped*
// keys is kept (up to `kMaxHistoryEntries` each), rather than a single
// snapshot, to survive the underlying key rotating between launches.
inline constexpr base::FilePath::CharType kOSCryptKeyBackupFileName[] =
    FILE_PATH_LITERAL("OSCrypt Key Backup");

// Every time a restore is actually attempted (as opposed to the common case
// where there's nothing to restore), its outcome is appended here - a
// durable, timestamped, append-only log. Local State only ever holds the
// live keys themselves, so a Local State pref would get silently overwritten
// or cleared right along with the thing it's describing; a separate file is
// the only way to keep a record of a restore having happened at all.
inline constexpr base::FilePath::CharType kOSCryptKeyRestoreFileName[] =
    FILE_PATH_LITERAL("OSCrypt Key Restore");

// What happened when the live key(s) were considered for backup this launch.
enum class OSCryptKeyBackupResult {
  kUnknown = 0,
  // At least one of the two keys' history gained a new distinct entry.
  kAppended = 1,
  // Both keys (that have a live value) already matched their newest history
  // entry; nothing was written.
  kUpToDate = 2,
  // Reading the backup file was fine (or it didn't exist), but writing the
  // updated backup back out failed.
  kWriteFailed = 3,
};

// Appends to the backup at `path` when the live key(s) are new. Blocking:
// unwraps `encrypted_key` (DPAPI, cheap and local) and, if `app_bound_key` is
// non-empty, unwraps it too (a round trip to the elevation service). Neither
// key's live value is compared to the backup's wrapped bytes - DPAPI and
// app-bound wrapping are both non-deterministic, so comparison happens on
// the unwrapped value. A key whose unwrapped value already matches the
// newest history entry on file is left alone; a genuinely new value is
// appended, evicting the oldest entry if the history would exceed
// `kMaxHistoryEntries`.
OSCryptKeyBackupResult AppendOSCryptKeyBackupIfNew(const base::FilePath& path,
                                                   std::string encrypted_key,
                                                   std::string app_bound_key);

// What happened on the restore path for one key. Appended to the restore
// log at `kOSCryptKeyRestoreFileName` (one value per key type per attempt);
// nothing is returned to the caller.
enum class OSCryptKeyRestoreResult {
  // `Local State` already had a key, or (for the app-bound-specific value)
  // no app-bound restore was attempted at all. Nothing was read from disk.
  kNotAttempted = 0,
  // The key was missing and there is no backup to put back.
  kNoBackup = 1,
  // The key was missing and a history entry was found and verified (it
  // actually unwrapped) before being put back.
  kRestoredVerified = 2,
  // The key was missing and a backup exists but could not be used (the file
  // is unreadable, or this key's history is empty).
  kBackupUnusable = 3,
  // The key was missing and no history entry could be verified. The newest
  // entry was put back anyway - OSCrypt's own new-key-minting fallback is
  // the safety net if it turns out not to work either.
  kRestoredUnverifiedFallback = 4,
};

// Guarding this feature so we can control w/ Griffin. Enabled by default.
// It's worth noting that the variations seed is also stored in `Local State`.
BASE_DECLARE_FEATURE(kBraveOSCryptKeyRestore);

// Puts a backed-up key back when `Local State` has lost it. Must run before
// OSCrypt initializes, since that is what reads the key and what mints a
// replacement when it finds none.
//
// Action is only taken when the DPAPI key is missing (value is missing or
// file is missing) - that is the outer gate for this whole function. The
// app-bound key is checked and restored independently (and only) within
// that same call, since the DPAPI key and the app-bound key belong to
// separate providers that replace them independently; app-bound is simply
// not attempted if the DPAPI key was present.
//
// For each key, history entries are verified newest-to-oldest (actually
// unwrapped) and the first one that verifies is restored; if none verify,
// the newest is restored anyway. Verifying an app-bound entry blocks this
// (UI) thread on a round trip to the elevation service, since this runs
// before any message loop exists to bridge asynchronously - see
// os_crypt_key_backup_com_bridge.h.
//
// Whenever either key is actually attempted, a record of both keys'
// outcomes (one may be `kNotAttempted`) is appended to the restore log at
// `kOSCryptKeyRestoreFileName`.
void MaybeRestoreOSCryptKey(const base::FilePath& user_data_dir,
                            PrefService* local_state);

// Appends to the backup if the live key(s) are new, on a blocking background
// task. Does nothing until a key exists to copy.
void BackUpOSCryptKey(const base::FilePath& user_data_dir,
                      PrefService* local_state);

}  // namespace brave

#endif  // BRAVE_BROWSER_OS_CRYPT_OS_CRYPT_KEY_BACKUP_H_
