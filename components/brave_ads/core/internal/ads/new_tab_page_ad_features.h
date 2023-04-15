/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NEW_TAB_PAGE_AD_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NEW_TAB_PAGE_AD_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads::new_tab_page_ads {

BASE_DECLARE_FEATURE(kAdsFeature);

bool IsEnabled();

constexpr base::FeatureParam<int> kMaximumAdsPerHour{&kAdsFeature,
                                                     "maximum_ads_per_hour", 4};

constexpr base::FeatureParam<int> kMaximumAdsPerDay{&kAdsFeature,
                                                    "maximum_ads_per_day", 20};

constexpr base::FeatureParam<base::TimeDelta> kMinimumWaitTime{
    &kAdsFeature, "minimum_wait_time", base::Minutes(5)};

}  // namespace brave_ads::new_tab_page_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NEW_TAB_PAGE_AD_FEATURES_H_
