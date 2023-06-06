/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_COMMON_BRAVE_ADS_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_COMMON_BRAVE_ADS_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kBraveAdsFeature);

bool IsBraveAdsFeatureEnabled();

// Set to |true| to launch as an in process service.
constexpr base::FeatureParam<bool> kShouldLaunchAsInProcessRunService{
    &kBraveAdsFeature, "should_launch_as_in_process_service", false};

// Set to |true| to always run the ads service, even if Brave Private Ads are
// disabled.
constexpr base::FeatureParam<bool> kShouldAlwaysRunService{
    &kBraveAdsFeature, "should_always_run_service", false};

// Set to |true| to always trigger new tab page ad events even if Brave Private
// Ads are disabled. |kShouldAlwaysRunService| must be set to |true|, otherwise
// this feature param will be ignored.
constexpr base::FeatureParam<bool> kShouldAlwaysTriggerNewTabPageAdEvents{
    &kBraveAdsFeature, "should_always_trigger_new_tab_page_ad_events", false};

// Set to |true| to always trigger search result ad events even if Brave Private
// Ads are disabled. |kShouldAlwaysRunService| must be set to |true|, otherwise
// this feature param will be ignored.
constexpr base::FeatureParam<bool> kShouldAlwaysTriggerSearchResultAdEvents{
    &kBraveAdsFeature, "should_always_trigger_search_result_ad_events", false};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_COMMON_BRAVE_ADS_FEATURE_H_
