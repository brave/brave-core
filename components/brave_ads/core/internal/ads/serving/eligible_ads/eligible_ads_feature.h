/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ELIGIBLE_ADS_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ELIGIBLE_ADS_FEATURE_H_

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kEligibleAdFeature);

bool IsEligibleAdFeatureEnabled();

constexpr base::FeatureParam<std::string> kAdPredictorWeights{
    &kEligibleAdFeature, "ad_predictor_weights",
    "1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0"};

constexpr base::FeatureParam<int> kBrowsingHistoryMaxCount{
    &kEligibleAdFeature, "browsing_history_max_count", 5'000};

constexpr base::FeatureParam<int> kBrowsingHistoryDaysAgo{
    &kEligibleAdFeature, "browsing_history_days_ago", 180};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ELIGIBLE_ADS_FEATURE_H_
