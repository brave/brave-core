/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_UTILS_H_
#define BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_UTILS_H_

#include "base/functional/callback_forward.h"

namespace geolocation {

// Run |callback| with true when system location service is available to
// applications.
void IsSystemLocationSettingEnabled(base::OnceCallback<void(bool)> callback);

bool CanGiveDetailedGeolocationRequestInfo();

}  // namespace geolocation

#endif  // BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_UTILS_H_
