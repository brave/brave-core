/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kCreativeAdModelBasedPredictorFeature);

constexpr base::FeatureParam<double> kChildIntentSegmentAdPredictorWeight{
    &kCreativeAdModelBasedPredictorFeature,
    "child_intent_segment_ad_predictor_weight", 1.0};
constexpr base::FeatureParam<double> kParentIntentSegmentAdPredictorWeight{
    &kCreativeAdModelBasedPredictorFeature,
    "parent_intent_segment_ad_predictor_weight", 1.0};

constexpr base::FeatureParam<double>
    kChildLatentInterestSegmentAdPredictorWeight{
        &kCreativeAdModelBasedPredictorFeature,
        "child_latent_interest_segment_ad_predictor_weight", 1.0};
constexpr base::FeatureParam<double>
    kParentLatentInterestSegmentAdPredictorWeight{
        &kCreativeAdModelBasedPredictorFeature,
        "parent_latent_interest_segment_ad_predictor_weight", 1.0};

constexpr base::FeatureParam<double> kChildInterestSegmentAdPredictorWeight{
    &kCreativeAdModelBasedPredictorFeature,
    "child_interest_segment_ad_predictor_weight", 1.0};
constexpr base::FeatureParam<double> kParentInterestSegmentAdPredictorWeight{
    &kCreativeAdModelBasedPredictorFeature,
    "parent_interest_segment_ad_predictor_weight", 1.0};

constexpr base::FeatureParam<double> kLastSeenAdPredictorWeight{
    &kCreativeAdModelBasedPredictorFeature, "last_seen_ad_predictor_weight",
    1.0};
constexpr base::FeatureParam<double> kLastSeenAdvertiserAdPredictorWeight{
    &kCreativeAdModelBasedPredictorFeature,
    "last_seen_advertiser_ad_predictor_weight", 1.0};

constexpr base::FeatureParam<double> kPriorityAdPredictorWeight{
    &kCreativeAdModelBasedPredictorFeature, "priority_ad_predictor_weight",
    1.0};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_AD_MODEL_BASED_PREDICTOR_FEATURE_H_
