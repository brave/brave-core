/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define NewSystemLocationProvider NewSystemLocationProvider_ChromiumImpl
#include <services/device/geolocation/location_provider_linux_portal.cc>
#undef NewSystemLocationProvider

#include <memory>

#include "services/device/geolocation/location_provider_manager.h"

namespace device {

// Brave defaults `kLocationProviderManagerParam` to `kPlatformOnly`, which on
// Linux now selects `LocationProviderLinuxPortal`. That provider is driven on
// the geolocation thread, but issues D-Bus calls on the shared session bus,
// whose origin thread is the UI thread, so it CHECKs in
// `dbus::Bus::AssertOnOriginThread()` as soon as the permission is granted.
// Keep Linux without a platform provider until the upstream threading is fixed
// (crbug.com/537480522).
std::unique_ptr<LocationProvider> NewSystemLocationProvider() {
  return nullptr;
}

}  // namespace device
