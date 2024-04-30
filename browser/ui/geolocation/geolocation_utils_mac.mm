/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/geolocation/geolocation_utils.h"

#import <CoreLocation/CoreLocation.h>

namespace geolocation {

bool IsSystemLocationSettingEnabled() {
  // Service is off globally.
  if (![CLLocationManager locationServicesEnabled]) {
    return false;
  }

  CLLocationManager* manager = [[CLLocationManager alloc] init];
  if (@available(macOS 11.0, *)) {
    auto status = [manager authorizationStatus];
    if (status == kCLAuthorizationStatusAuthorized) {
      return true;
    }
  }

  return false;
}

bool CanGiveDetailedGeolocationRequestInfo() {
  if (@available(macOS 11.0, *)) {
    return true;
  }

  return false;
}

}  // namespace geolocation
