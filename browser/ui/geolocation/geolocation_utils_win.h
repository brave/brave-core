/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_UTILS_WIN_H_
#define BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_UTILS_WIN_H_

namespace geolocation::win {

// True when system location service is available to applications.
bool IsSystemLocationSettingEnabled();

}  // namespace geolocation::win

#endif  // BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_UTILS_WIN_H_
