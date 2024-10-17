/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_DEVICE_ID_DEVICE_ID_IMPL_H_
#define BRAVE_BROWSER_BRAVE_ADS_DEVICE_ID_DEVICE_ID_IMPL_H_

#include "base/containers/span.h"
#include "brave/components/brave_ads/browser/device_id/device_id.h"

namespace brave_ads {

class DeviceIdImpl : public DeviceId {
 public:
  void GetDeviceId(DeviceIdCallback callback) override;

  // On some platforms, part of the machine ID is the MAC address. This function
  // is shared across platforms to filter out MAC addresses that have been
  // identified as invalid, i.e. not unique. For example, some VM hosts assign a
  // new MAC address at each reboot.
  static bool IsValidMacAddress(base::span<const uint8_t> bytes);

 private:
  // Platform specific implementation of "raw" device id retrieval.
  static void GetRawDeviceId(DeviceIdCallback callback);
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_DEVICE_ID_DEVICE_ID_IMPL_H_
