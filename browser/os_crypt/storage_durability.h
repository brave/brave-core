/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_OS_CRYPT_STORAGE_DURABILITY_H_
#define BRAVE_BROWSER_OS_CRYPT_STORAGE_DURABILITY_H_

#include <stdint.h>

#include "base/files/file_path.h"

class PrefRegistrySimple;
class PrefService;

namespace brave {

// Whether the drive holding the profile can be trusted to have actually written
// what Brave asked it to write.
//
// This is a different risk from the one in dpapi_risk.h. That file is about
// losing the DPAPI master key, where the data is intact but unreadable. This is
// about `Local State` being damaged, where the key itself is destroyed —
// `JsonPrefStore` then moves the file aside as `Local State.bad` and carries on
// with empty preferences, taking the encryption key with it.
//
// `JsonPrefStore` writes through `base::ImportantFileWriter`, which writes a
// temporary file and renames it over the original. The rename is atomic, so a
// killed process cannot tear the file. What it does not do is flush before
// renaming, so durability is left to the drive. If the drive is holding the
// write in a volatile cache when power is lost, the rename can survive while
// its contents do not, which is one of the ways an empty or truncated
// `Local State` appears.
//
// Windows exposes a per-device setting that makes this markedly worse. In
// Device Manager, under a disk's Policies tab, "Turn off Windows write-cache
// buffer flushing on the device" tells Windows to stop sending flush commands
// at all. Microsoft's own UI warns that this can cause data loss on power
// failure. It is surfaced to us as
// `STORAGE_WRITE_CACHE_PROPERTY::UserDefinedPowerProtection`, documented in
// winioctl.h as "User selected power protection option through registry".
//
// It is not exposed through WMI. The way to read it is `DeviceIoControl()` with
// `IOCTL_STORAGE_QUERY_PROPERTY` and `StorageDeviceWriteCacheProperty`.
//
// Note the setting is not always wrong: on hardware with a battery-backed cache
// it is the correct configuration, and the device reports that separately as
// `NVCacheEnabled`.

// Raw facts about the drive, recorded rather than interpreted, so that a later
// change can decide what they mean without us having baked a judgement into
// stored data.
enum class StorageDurabilitySignal : uint32_t {
  // The drive could not be asked. Nothing below should be read as meaningful.
  kQueryFailed = 1 << 0,
  // The drive is caching writes in volatile memory.
  kWriteCacheEnabled = 1 << 1,
  // "Turn off Windows write-cache buffer flushing on the device" is on.
  kUserDefinedPowerProtection = 1 << 2,
  // The cache is battery backed, which is what makes the setting above
  // legitimate rather than dangerous.
  kNVCacheEnabled = 1 << 3,
  // The drive reports that it cannot flush its cache on request.
  kFlushNotSupported = 1 << 4,
};

struct StorageDurability {
  bool queried = false;
  bool write_cache_enabled = false;
  bool user_defined_power_protection = false;
  bool nv_cache_enabled = false;
  bool flush_cache_supported = true;
};

uint32_t ComputeStorageDurabilitySignals(const StorageDurability& durability);

// Whether these signals describe a drive that can lose an already-renamed file
// on power loss. True only when writes are cached, Windows has been told not to
// flush them, and nothing is backing the cache up.
bool IsWriteCacheFlushingUnsafe(uint32_t signals);

// Asks the drive holding `path`. Blocking, and returns `queried == false` when
// the drive cannot be asked, which includes network paths and any volume that
// is not reached through a drive letter.
StorageDurability QueryStorageDurability(const base::FilePath& path);

void RegisterStorageDurabilityLocalStatePrefs(PrefRegistrySimple* registry);

// Queries on a blocking background task and records the result in local state.
// Nothing is reported anywhere and nothing is shown to the user.
void RecordStorageDurability(const base::FilePath& user_data_dir,
                             PrefService* local_state);

}  // namespace brave

#endif  // BRAVE_BROWSER_OS_CRYPT_STORAGE_DURABILITY_H_
