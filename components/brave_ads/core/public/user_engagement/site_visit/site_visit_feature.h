/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

BASE_DECLARE_FEATURE(kSiteVisitFeature);

inline constexpr base::FeatureParam<base::TimeDelta> kPageLandAfter{
    &kSiteVisitFeature, "page_land_after", base::Seconds(5)};

// Set to 0 to never cap.
inline constexpr base::FeatureParam<int> kPageLandCap{&kSiteVisitFeature,
                                                      "page_land_cap", 0};

inline constexpr base::FeatureParam<bool> kShouldSuspendAndResumePageLand{
    &kSiteVisitFeature, "should_suspend_and_resume_page_land", true};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_FEATURE_H_
