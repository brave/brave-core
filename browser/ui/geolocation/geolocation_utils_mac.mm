/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/geolocation/geolocation_utils.h"

#import <CoreLocation/CoreLocation.h>

#include <utility>

#include "base/functional/callback.h"

namespace geolocation {

namespace {

bool GetSystemLocationSettingEnabled() {
  // Service is off globally.
  if (![CLLocationManager locationServicesEnabled]) {
    return false;
  }

  if (@available(macOS 11.0, *)) {
    CLLocationManager* manager = [[CLLocationManager alloc] init];
    if ([manager authorizationStatus] ==
        kCLAuthorizationStatusAuthorizedAlways) {
      return true;
    }
  }

  return false;
}

}  // namespace

void IsSystemLocationSettingEnabled(base::OnceCallback<void(bool)> callback) {
  std::move(callback).Run(GetSystemLocationSettingEnabled());
}

bool CanGiveDetailedGeolocationRequestInfo() {
  if (@available(macOS 11.0, *)) {
    return true;
  }

  return false;
}

}  // namespace geolocation
