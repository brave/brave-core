/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/sync_device_info/device_info_prefs.h"

#define RegisterProfilePrefs RegisterProfilePrefs_ChromiumImpl

#include "src/components/sync_device_info/device_info_prefs.cc"

#undef RegisterProfilePrefs

namespace syncer {
namespace {

// Preference name for storing the time when reset of progress token for devices
// was done. This happens when we need to re-fetch the devices which were
// expired and are hidden on the client but still present on the server.
constexpr char kResetDevicesProgressTokenTime[] =
    "brave_sync_v2.reset_devices_progress_token_time";

}  // namespace

bool DeviceInfoPrefs::IsResetDevicesProgressTokenDone() {
  base::Time time = pref_service_->GetTime(kResetDevicesProgressTokenTime);
  return !time.is_null();
}

void DeviceInfoPrefs::SetResetDevicesProgressTokenDone() {
  pref_service_->SetTime(kResetDevicesProgressTokenTime, base::Time::Now());
}

void DeviceInfoPrefs::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kResetDevicesProgressTokenTime, base::Time());
  RegisterProfilePrefs_ChromiumImpl(registry);
}

}  // namespace syncer
