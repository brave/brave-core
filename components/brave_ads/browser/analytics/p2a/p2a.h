/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ANALYTICS_P2A_P2A_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ANALYTICS_P2A_P2A_H_

#include <string_view>

class PrefService;
class PrefRegistrySimple;

namespace brave_ads {

void RegisterP2APrefs(PrefRegistrySimple* registry);

void RecordAndEmitP2AHistogramName(PrefService* prefs, std::string_view name);

void SuspendP2AHistograms();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ANALYTICS_P2A_P2A_H_
