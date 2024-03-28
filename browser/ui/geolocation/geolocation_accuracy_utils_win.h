/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_ACCURACY_UTILS_WIN_H_
#define BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_ACCURACY_UTILS_WIN_H_

// True when system location service is available to applications.
bool IsSystemLocationSettingEnabled();
void LaunchLocationServiceSettings();

#endif  // BRAVE_BROWSER_UI_GEOLOCATION_GEOLOCATION_ACCURACY_UTILS_WIN_H_
