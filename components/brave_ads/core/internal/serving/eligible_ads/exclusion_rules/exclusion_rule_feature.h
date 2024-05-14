/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

BASE_DECLARE_FEATURE(kExclusionRulesFeature);

// Set to 0 to never exclude an ad if dismissed within the time window.
inline constexpr base::FeatureParam<base::TimeDelta>
    kShouldExcludeAdIfDismissedWithinTimeWindow{
        &kExclusionRulesFeature,
        "should_exclude_ad_if_dismissed_within_time_window", base::Hours(0)};

// Set to 0 to never exclude an ad if landed within the time window.
inline constexpr base::FeatureParam<base::TimeDelta>
    kShouldExcludeAdIfLandedOnPageWithinTimeWindow{
        &kExclusionRulesFeature,
        "should_exclude_ad_if_landed_on_page_within_time_window",
        base::Hours(0)};

// Set to 0 to never exclude an ad if the creative instance was shown within the
// time window.
inline constexpr base::FeatureParam<base::TimeDelta>
    kShouldExcludeAdIfCreativeInstanceWithinTimeWindow{
        &kExclusionRulesFeature,
        "should_exclude_ad_if_creative_instance_within_time_window",
        base::Hours(1)};

// Set to 0 to never exclude an ad if the creative instance exceeds the per hour
// cap.
inline constexpr base::FeatureParam<int>
    kShouldExcludeAdIfCreativeInstanceExceedsPerHourCap{
        &kExclusionRulesFeature,
        "should_exclude_ad_if_creative_instance_exceeds_per_hour_cap", 1};

// Set to 0 to never exclude an ad if the creative set exceeds the conversion
// cap.
inline constexpr base::FeatureParam<int>
    kShouldExcludeAdIfCreativeSetExceedsConversionCap{
        &kExclusionRulesFeature,
        "should_exclude_ad_if_creative_set_exceeds_conversion_cap", 1};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_EXCLUSION_RULE_FEATURE_H_
