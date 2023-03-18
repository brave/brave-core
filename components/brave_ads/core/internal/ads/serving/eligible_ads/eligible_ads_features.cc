/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_features.h"

#include <iterator>
#include <string>

#include "base/metrics/field_trial_params.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_features_util.h"

namespace brave_ads::features {

namespace {

constexpr char kAdPredictorWeightsFieldTrialParamName[] =
    "ad_predictor_weights";
constexpr double kAdPredictorWeightsDefaultValue[] = {
    /*kDoesMatchIntentChildSegmentsIndex*/ 1.0,
    /*kDoesMatchIntentParentSegmentsIndex*/ 1.0,
    /*kDoesMatchInterestChildSegmentsIndex*/ 1.0,
    /*kDoesMatchInterestParentSegmentsIndex*/ 1.0,
    /*AdLastSeenHoursAgoIndex*/ 1.0,
    /*kAdvertiserLastSeenHoursAgoIndex*/ 1.0,
    /*kPriorityIndex*/ 1.0};

constexpr char kBrowsingHistoryMaxCountFieldTrialParamName[] =
    "browsing_history_max_count";
constexpr int kBrowsingHistoryMaxCountDefaultValue = 5'000;

constexpr char kBrowsingHistoryDaysAgoFieldTrialParamName[] =
    "browsing_history_days_ago";
constexpr int kBrowsingHistoryDaysAgoDefaultValue = 180;

}  // namespace

BASE_FEATURE(kEligibleAds, "EligibleAds", base::FEATURE_ENABLED_BY_DEFAULT);

bool IsEligibleAdsEnabled() {
  return base::FeatureList::IsEnabled(kEligibleAds);
}

AdPredictorWeightList GetAdPredictorWeights() {
  const std::string field_trial_param_value = GetFieldTrialParamValueByFeature(
      kEligibleAds, kAdPredictorWeightsFieldTrialParamName);

  AdPredictorWeightList predictor_weights =
      ToAdPredictorWeights(field_trial_param_value);
  if (predictor_weights.empty()) {
    base::ranges::copy(kAdPredictorWeightsDefaultValue,
                       std::back_inserter(predictor_weights));
  }

  return predictor_weights;
}

int GetBrowsingHistoryMaxCount() {
  return GetFieldTrialParamByFeatureAsInt(
      kEligibleAds, kBrowsingHistoryMaxCountFieldTrialParamName,
      kBrowsingHistoryMaxCountDefaultValue);
}

int GetBrowsingHistoryDaysAgo() {
  return GetFieldTrialParamByFeatureAsInt(
      kEligibleAds, kBrowsingHistoryDaysAgoFieldTrialParamName,
      kBrowsingHistoryDaysAgoDefaultValue);
}

}  // namespace brave_ads::features
