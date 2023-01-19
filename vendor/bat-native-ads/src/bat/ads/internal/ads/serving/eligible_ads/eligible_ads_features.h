/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ELIGIBLE_ADS_FEATURES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ELIGIBLE_ADS_FEATURES_H_

#include "base/feature_list.h"  // IWYU pragma: keep
#include "bat/ads/internal/ads/serving/eligible_ads/eligible_ads_alias.h"

namespace ads::features {

BASE_DECLARE_FEATURE(kEligibleAds);

bool IsEligibleAdsEnabled();

AdPredictorWeightList GetAdPredictorWeights();

}  // namespace ads::features

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_ELIGIBLE_ADS_FEATURES_H_
