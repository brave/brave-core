/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/device_id/device_id_impl.h"

#include <utility>

#include "base/callback.h"

namespace brave_ads {

// static
void DeviceIdImpl::GetRawDeviceId(DeviceIdCallback callback) {
  std::move(callback).Run({});
}

}  // namespace brave_ads
