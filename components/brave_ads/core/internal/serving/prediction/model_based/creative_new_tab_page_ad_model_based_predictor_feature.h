/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_NEW_TAB_PAGE_AD_MODEL_BASED_PREDICTOR_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_NEW_TAB_PAGE_AD_MODEL_BASED_PREDICTOR_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kCreativeNewTabPageAdModelBasedPredictorFeature);

inline constexpr base::FeatureParam<double>
    kNewTabPageAdChildIntentSegmentPredictorWeight{
        &kCreativeNewTabPageAdModelBasedPredictorFeature,
        "child_intent_segment_predictor_weight", 0.0};
inline constexpr base::FeatureParam<double>
    kNewTabPageAdParentIntentSegmentPredictorWeight{
        &kCreativeNewTabPageAdModelBasedPredictorFeature,
        "parent_intent_segment_predictor_weight", 0.0};

inline constexpr base::FeatureParam<double>
    kNewTabPageAdChildLatentInterestSegmentPredictorWeight{
        &kCreativeNewTabPageAdModelBasedPredictorFeature,
        "child_latent_interest_segment_predictor_weight", 0.0};
inline constexpr base::FeatureParam<double>
    kNewTabPageAdParentLatentInterestSegmentPredictorWeight{
        &kCreativeNewTabPageAdModelBasedPredictorFeature,
        "parent_latent_interest_segment_predictor_weight", 0.0};

inline constexpr base::FeatureParam<double>
    kNewTabPageAdChildInterestSegmentPredictorWeight{
        &kCreativeNewTabPageAdModelBasedPredictorFeature,
        "child_interest_segment_predictor_weight", 0.0};
inline constexpr base::FeatureParam<double>
    kNewTabPageAdParentInterestSegmentPredictorWeight{
        &kCreativeNewTabPageAdModelBasedPredictorFeature,
        "parent_interest_segment_predictor_weight", 0.0};

inline constexpr base::FeatureParam<double>
    kNewTabPageAdUntargetedSegmentPredictorWeight{
        &kCreativeNewTabPageAdModelBasedPredictorFeature,
        "untargeted_segment_predictor_weight", 0.0001};

inline constexpr base::FeatureParam<double>
    kNewTabPageAdLastSeenPredictorWeight{
        &kCreativeNewTabPageAdModelBasedPredictorFeature,
        "last_seen_ad_predictor_weight", 1.0};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_PREDICTION_MODEL_BASED_CREATIVE_NEW_TAB_PAGE_AD_MODEL_BASED_PREDICTOR_FEATURE_H_
