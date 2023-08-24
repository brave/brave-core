/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(CreativeAdModelBasedPredictorFeature);

constexpr base::FeatureParam<double> kChildIntentSegmentAdPredictorWeight{
    &CreativeAdModelBasedPredictorFeature,
    "child_intent_segment_ad_predictor_weight", 1.0};
constexpr base::FeatureParam<double> kParentIntentSegmentAdPredictorWeight{
    &CreativeAdModelBasedPredictorFeature,
    "parent_intent_segment_ad_predictor_weight", 1.0};

constexpr base::FeatureParam<double>
    kChildLatentInterestSegmentAdPredictorWeight{
        &CreativeAdModelBasedPredictorFeature,
        "child_latent_interest_segment_ad_predictor_weight", 1.0};
constexpr base::FeatureParam<double>
    kParentLatentInterestSegmentAdPredictorWeight{
        &CreativeAdModelBasedPredictorFeature,
        "parent_latent_interest_segment_ad_predictor_weight", 1.0};

constexpr base::FeatureParam<double> kChildInterestSegmentAdPredictorWeight{
    &CreativeAdModelBasedPredictorFeature,
    "child_interest_segment_ad_predictor_weight", 1.0};
constexpr base::FeatureParam<double> kParentInterestSegmentAdPredictorWeight{
    &CreativeAdModelBasedPredictorFeature,
    "parent_interest_segment_ad_predictor_weight", 1.0};

constexpr base::FeatureParam<double> kLastSeenAdPredictorWeight{
    &CreativeAdModelBasedPredictorFeature, "last_seen_ad_predictor_weight",
    1.0};
constexpr base::FeatureParam<double> kLastSeenAdvertiserAdPredictorWeight{
    &CreativeAdModelBasedPredictorFeature,
    "last_seen_advertiser_ad_predictor_weight", 1.0};

constexpr base::FeatureParam<double> kPriorityAdPredictorWeight{
    &CreativeAdModelBasedPredictorFeature, "priority_ad_predictor_weight", 1.0};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_FEATURE_H_
