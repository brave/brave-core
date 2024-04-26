/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_INLINE_CONTENT_AD_MODEL_BASED_PREDICTOR_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_INLINE_CONTENT_AD_MODEL_BASED_PREDICTOR_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kCreativeInlineContentAdModelBasedPredictorFeature);

inline constexpr base::FeatureParam<double>
    kInlineContentAdChildIntentSegmentPredictorWeight{
        &kCreativeInlineContentAdModelBasedPredictorFeature,
        "child_intent_segment_predictor_weight", 0.0};
inline constexpr base::FeatureParam<double>
    kInlineContentAdParentIntentSegmentPredictorWeight{
        &kCreativeInlineContentAdModelBasedPredictorFeature,
        "parent_intent_segment_predictor_weight", 0.0};

inline constexpr base::FeatureParam<double>
    kInlineContentAdChildLatentInterestSegmentPredictorWeight{
        &kCreativeInlineContentAdModelBasedPredictorFeature,
        "child_latent_interest_segment_predictor_weight", 0.0};
inline constexpr base::FeatureParam<double>
    kInlineContentAdParentLatentInterestSegmentPredictorWeight{
        &kCreativeInlineContentAdModelBasedPredictorFeature,
        "parent_latent_interest_segment_predictor_weight", 0.0};

inline constexpr base::FeatureParam<double>
    kInlineContentAdChildInterestSegmentPredictorWeight{
        &kCreativeInlineContentAdModelBasedPredictorFeature,
        "child_interest_segment_predictor_weight", 0.0};
inline constexpr base::FeatureParam<double>
    kInlineContentAdParentInterestSegmentPredictorWeight{
        &kCreativeInlineContentAdModelBasedPredictorFeature,
        "parent_interest_segment_predictor_weight", 0.0};

inline constexpr base::FeatureParam<double>
    kInlineContentAdUntargetedSegmentPredictorWeight{
        &kCreativeInlineContentAdModelBasedPredictorFeature,
        "untargeted_segment_predictor_weight", 0.0001};

inline constexpr base::FeatureParam<double>
    kInlineContentAdLastSeenPredictorWeight{
        &kCreativeInlineContentAdModelBasedPredictorFeature,
        "last_seen_ad_predictor_weight", 1.0};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_INLINE_CONTENT_AD_MODEL_BASED_PREDICTOR_FEATURE_H_
