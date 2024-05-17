/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

BASE_DECLARE_FEATURE(kNewTabPageAdFeature);

// Set to 0 to never cap.
inline constexpr base::FeatureParam<int> kMaximumNewTabPageAdsPerHour{
    &kNewTabPageAdFeature, "maximum_ads_per_hour", 4};

// Set to 0 to never cap.
inline constexpr base::FeatureParam<int> kMaximumNewTabPageAdsPerDay{
    &kNewTabPageAdFeature, "maximum_ads_per_day", 20};

// Set to 0 to never cap.
inline constexpr base::FeatureParam<base::TimeDelta>
    kNewTabPageAdMinimumWaitTime{&kNewTabPageAdFeature, "minimum_wait_time",
                                 base::Minutes(5)};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_FEATURE_H_
