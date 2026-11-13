/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_FEATURE_H_

#include "base/feature.h"
#include "base/metrics/field_trial_params.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

BASE_DECLARE_FEATURE(kConversionsFeature);

inline constexpr base::FeatureParam<int> kConversionResourceVersion{
    &kConversionsFeature, "resource_version", 1};

// Set to 0 to never cap campaign conversions.
inline constexpr base::FeatureParam<size_t> kCampaignConversionCap{
    &kConversionsFeature, "campaign_conversion_cap", 0};

// The maximum time to look back when fetching ad events for potential
// conversions.
inline constexpr base::FeatureParam<base::TimeDelta> kAdEventsTimeWindow{
    &kConversionsFeature, "ad_events_time_window", base::Days(30)};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_FEATURE_H_
