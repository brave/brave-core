/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/eligible_ads_features.h"

#include <string>

#include "base/metrics/field_trial_params.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_features_util.h"

namespace ads {
namespace features {

namespace {

const char kFeatureName[] = "EligibleAds";
const char kFieldTrialParameterAdPredictorWeights[] = "ad_predictor_weights";
const AdPredictorWeights kDefaultWeights = {
    /* kDoesMatchIntentChildSegmentsIndex */ 1.0,
    /* kDoesMatchIntentParentSegmentsIndex */ 1.0,
    /* kDoesMatchInterestChildSegmentsIndex */ 1.0,
    /* kDoesMatchInterestParentSegmentsIndex */ 1.0,
    /* AdLastSeenHoursAgoIndex */ 1.0,
    /* kAdvertiserLastSeenHoursAgoIndex */ 1.0,
    /* kPriorityIndex */ 1.0};

}  // namespace

const base::Feature kEligibleAds{kFeatureName,
                                 base::FEATURE_ENABLED_BY_DEFAULT};

bool IsEligibleAdsEnabled() {
  return base::FeatureList::IsEnabled(kEligibleAds);
}

AdPredictorWeights GetAdPredictorWeights() {
  const std::string param_value = GetFieldTrialParamValueByFeature(
      kEligibleAds, kFieldTrialParameterAdPredictorWeights);

  AdPredictorWeights weights = ToAdPredictorWeights(param_value);
  if (weights.empty()) {
    weights = kDefaultWeights;
  }

  return weights;
}

}  // namespace features
}  // namespace ads
