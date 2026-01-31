/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_VIRTUAL_PREF_VIRTUAL_PREF_PROVIDER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_VIRTUAL_PREF_VIRTUAL_PREF_PROVIDER_UTIL_H_

namespace brave_ads {

int GetMajorVersion();
int GetMinorVersion();
int GetBuildVersion();
int GetPatchVersion();

bool IsMobilePlatform();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_VIRTUAL_PREF_VIRTUAL_PREF_PROVIDER_UTIL_H_
