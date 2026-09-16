/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/os_crypt/storage_durability.h"

#include <windows.h>

#include <winioctl.h>

#include <cstddef>
#include <string>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/strcat.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/win/scoped_handle.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave {

namespace {

constexpr char kSignalsPrefName[] = "brave.os_crypt.storage_durability_signals";
constexpr char kLastComputedPrefName[] =
    "brave.os_crypt.storage_durability_last_computed";

// Opens the volume `path` sits on, for querying only. Zero access rights are
// requested deliberately: `IOCTL_STORAGE_QUERY_PROPERTY` does not need read
// access, and asking for it would require privileges Brave does not have.
base::win::ScopedHandle OpenVolumeForQuery(const base::FilePath& path) {
  const std::wstring& value = path.value();
  // Only a drive-letter path identifies a volume we can name this way. A UNC
  // path is a network share, which is recorded as its own risk elsewhere and
  // has no local write cache to ask about.
  if (value.size() < 2 || value[1] != L':') {
    return base::win::ScopedHandle();
  }

  const std::wstring volume_path =
      base::StrCat({LR"(\\.\)", value.substr(0, 2)});
  return base::win::ScopedHandle(
      ::CreateFile(volume_path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                   nullptr, OPEN_EXISTING, 0, nullptr));
}

void OnDurabilityQueried(PrefService* local_state,
                         const StorageDurability& durability) {
  local_state->SetInteger(
      kSignalsPrefName,
      static_cast<int>(ComputeStorageDurabilitySignals(durability)));
  local_state->SetTime(kLastComputedPrefName, base::Time::Now());
}

}  // namespace

uint32_t ComputeStorageDurabilitySignals(const StorageDurability& durability) {
  if (!durability.queried) {
    return static_cast<uint32_t>(StorageDurabilitySignal::kQueryFailed);
  }

  uint32_t signals = 0;
  auto add = [&signals](bool present, StorageDurabilitySignal signal) {
    if (present) {
      signals |= static_cast<uint32_t>(signal);
    }
  };

  add(durability.write_cache_enabled,
      StorageDurabilitySignal::kWriteCacheEnabled);
  add(durability.user_defined_power_protection,
      StorageDurabilitySignal::kUserDefinedPowerProtection);
  add(durability.nv_cache_enabled, StorageDurabilitySignal::kNVCacheEnabled);
  add(!durability.flush_cache_supported,
      StorageDurabilitySignal::kFlushNotSupported);

  return signals;
}

bool IsWriteCacheFlushingUnsafe(uint32_t signals) {
  constexpr uint32_t kFailed =
      static_cast<uint32_t>(StorageDurabilitySignal::kQueryFailed);
  constexpr uint32_t kCached =
      static_cast<uint32_t>(StorageDurabilitySignal::kWriteCacheEnabled);
  constexpr uint32_t kProtected = static_cast<uint32_t>(
      StorageDurabilitySignal::kUserDefinedPowerProtection);
  constexpr uint32_t kNVBacked =
      static_cast<uint32_t>(StorageDurabilitySignal::kNVCacheEnabled);

  if (signals & kFailed) {
    return false;
  }
  // A battery-backed cache is what makes the Device Manager setting a
  // reasonable choice rather than a dangerous one, so it clears the risk.
  if (signals & kNVBacked) {
    return false;
  }
  return (signals & kCached) && (signals & kProtected);
}

StorageDurability QueryStorageDurability(const base::FilePath& path) {
  StorageDurability durability;

  base::win::ScopedHandle volume = OpenVolumeForQuery(path);
  if (!volume.is_valid()) {
    return durability;
  }

  STORAGE_PROPERTY_QUERY query = {};
  query.PropertyId = StorageDeviceWriteCacheProperty;
  query.QueryType = PropertyStandardQuery;

  STORAGE_WRITE_CACHE_PROPERTY result = {};
  // Everything through `NVCacheEnabled`; the struct's trailing
  // padding is not required of the driver.
  constexpr DWORD kMinimumResultSize =
      offsetof(STORAGE_WRITE_CACHE_PROPERTY, NVCacheEnabled) + 1;
  DWORD bytes_returned = 0;
  if (!::DeviceIoControl(volume.get(), IOCTL_STORAGE_QUERY_PROPERTY, &query,
                         sizeof(query), &result, sizeof(result),
                         &bytes_returned, nullptr) ||
      bytes_returned < kMinimumResultSize) {
    // Not every driver answers this, so an unhelpful drive is expected rather
    // than exceptional.
    return durability;
  }

  durability.queried = true;
  durability.write_cache_enabled =
      result.WriteCacheEnabled == WriteCacheEnabled;
  durability.user_defined_power_protection =
      result.UserDefinedPowerProtection != FALSE;
  durability.nv_cache_enabled = result.NVCacheEnabled != FALSE;
  durability.flush_cache_supported = result.FlushCacheSupported != FALSE;

  return durability;
}

void RegisterStorageDurabilityLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kSignalsPrefName, 0);
  registry->RegisterTimePref(kLastComputedPrefName, base::Time());
}

void RecordStorageDurability(const base::FilePath& user_data_dir,
                             PrefService* local_state) {
  if (user_data_dir.empty() || !local_state) {
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&QueryStorageDurability, user_data_dir),
      // `local_state` is owned by BrowserProcessImpl and outlives every task
      // posted here; SKIP_ON_SHUTDOWN keeps this from running during teardown.
      base::BindOnce(&OnDurabilityQueried, base::Unretained(local_state)));
}

}  // namespace brave
