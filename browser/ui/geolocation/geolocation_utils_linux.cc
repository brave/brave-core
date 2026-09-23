/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/geolocation/geolocation_utils.h"

#include "base/functional/callback.h"

namespace geolocation {

void IsSystemLocationSettingEnabled(base::OnceCallback<void(bool)> callback) {}

bool CanGiveDetailedGeolocationRequestInfo() {
  return false;
}

}  // namespace geolocation
