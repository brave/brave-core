/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_REGISTRY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_REGISTRY_H_

class PrefRegistrySimple;

namespace brave_ads {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_REGISTRY_H_
