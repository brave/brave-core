/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

BASE_DECLARE_FEATURE(kAdEventFeature);

// Set to 0 to always debounce viewed ad events.
inline constexpr base::FeatureParam<base::TimeDelta> kDebounceViewedAdEventFor{
    &kAdEventFeature, "debounce_viewed_ad_event_for", base::Seconds(0)};

// Set to 0 to always debounce clicked ad events.
inline constexpr base::FeatureParam<base::TimeDelta> kDebounceClickedAdEventFor{
    &kAdEventFeature, "debounce_clicked_ad_event_for", base::Seconds(0)};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_FEATURE_H_
