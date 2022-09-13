/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_DEVICE_ID_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_DEVICE_ID_H_

#include <string>

#include "base/callback_forward.h"

namespace brave_ads {

using DeviceIdCallback = base::OnceCallback<void(std::string)>;

class DeviceId {
 public:
  DeviceId();
  virtual ~DeviceId();
  DeviceId(const DeviceId&) = delete;
  DeviceId& operator=(const DeviceId&) = delete;

  // Calls |callback| with a unique device identifier as argument. The device
  // identifier has the following characteristics:
  // 1. It is shared across users of a device.
  // 2. It is resilient to device reboots.
  // 3. It can be reset in *some* way by the user. In Particular, it can *not*
  //    be based only on a MAC address of a physical device. The specific
  //    implementation varies across platforms, some of them requiring a round
  //    trip to the IO or FILE thread. "callback" will always be called on the
  //    UI thread though (sometimes directly if the implementation allows
  //    running on the UI thread). The returned value is
  //    HMAC_SHA256(|raw_device_id|), so that the actual device identifier value
  //    is not exposed directly to the caller.
  //
  // NOTE: This device id must never leave the device.
  virtual void GetDeviceId(DeviceIdCallback callback) = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_DEVICE_ID_H_
