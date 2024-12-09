/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_FEATURE_H_

#include "base/feature_list.h"

namespace brave_ads {

// Set to `true` to always run the ads service, even if Brave Private Ads are
// disabled.
BASE_DECLARE_FEATURE(kShouldAlwaysRunBraveAdsServiceFeature);

bool ShouldAlwaysRunService();

// Set to `true` to always trigger new tab page ad events even if Brave Private
// Ads are disabled. `ShouldAlwaysRunService()` must be set to `true`, otherwise
// this feature param will be ignored.
BASE_DECLARE_FEATURE(kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

bool ShouldAlwaysTriggerNewTabPageAdEvents();

// Set to `true` to support search result ads. `ShouldAlwaysRunService()` must
// be set to `true`, otherwise this feature param will be ignored.
BASE_DECLARE_FEATURE(kShouldSupportSearchResultAdsFeature);

bool ShouldSupportSearchResultAds();

// Set to `true` to always trigger search result ad events even if Brave Private
// Ads are disabled. `ShouldAlwaysRunService()` must be set to `true`, otherwise
// this feature param will be ignored.
BASE_DECLARE_FEATURE(kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

bool ShouldAlwaysTriggerSearchResultAdEvents();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_FEATURE_H_
