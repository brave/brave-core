/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ELIGIBLE_ADS_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ELIGIBLE_ADS_FEATURES_H_

#include "base/feature_list.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_alias.h"

namespace brave_ads::features {

BASE_DECLARE_FEATURE(kEligibleAds);

bool IsEligibleAdsEnabled();

AdPredictorWeightList GetAdPredictorWeights();

int GetBrowsingHistoryMaxCount();
int GetBrowsingHistoryDaysAgo();

}  // namespace brave_ads::features

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ELIGIBLE_ADS_FEATURES_H_
