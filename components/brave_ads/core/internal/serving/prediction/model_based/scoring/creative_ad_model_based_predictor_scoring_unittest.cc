/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/creative_ad_model_based_predictor_scoring.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor_feature.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeAdPredictorScoringTest : public UnitTestBase {};

TEST_F(BraveAdsCreativeAdPredictorScoringTest,
       ComputeCreativeAdPredictorScoreForDefaultWeights) {
  // Arrange
  CreativeAdInfo creative_ad =
      test::BuildCreativeAd(/*should_use_random_uuids=*/true);
  creative_ad.segment = "parent-child";

  const UserModelInfo user_model{
      IntentUserModelInfo{SegmentList{"parent-child"}},
      LatentInterestUserModelInfo{SegmentList{"parent-child"}},
      InterestUserModelInfo{SegmentList{"parent-child"},
                            TextEmbeddingHtmlEventList{}}};

  AdEventList ad_events;
  const AdEventInfo ad_event =
      test::BuildAdEvent(creative_ad, AdType::kNotificationAd,
                         ConfirmationType::kViewed, Now() - base::Hours(7),
                         /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const CreativeAdPredictorInputVariableInfo input_variable =
      ComputeCreativeAdPredictorInputVariable(creative_ad, user_model,
                                              ad_events);

  // Act & Assert
  EXPECT_DOUBLE_EQ(4.083333333333333, ComputeCreativeAdPredictorScore(
                                          creative_ad, input_variable));
}

TEST_F(BraveAdsCreativeAdPredictorScoringTest,
       ComputeCreativeAdPredictorScoreForNonDefaultWeights) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeAdModelBasedPredictorFeature,
      {{"child_intent_segment_ad_predictor_weight", "0.9"},
       {"parent_intent_segment_ad_predictor_weight", "0.8"},
       {"child_latent_interest_segment_ad_predictor_weight", "0.7"},
       {"parent_latent_interest_segment_ad_predictor_weight", "0.6"},
       {"child_interest_segment_ad_predictor_weight", "0.5"},
       {"parent_interest_segment_ad_predictor_weight", "0.4"},
       {"last_seen_ad_predictor_weight", "0.3"},
       {"last_seen_advertiser_ad_predictor_weight", "0.2"},
       {"priority_ad_predictor_weight", "0.1"}});

  CreativeAdInfo creative_ad =
      test::BuildCreativeAd(/*should_use_random_uuids=*/true);
  creative_ad.segment = "parent-child";

  const UserModelInfo user_model{
      IntentUserModelInfo{SegmentList{"parent-child"}},
      LatentInterestUserModelInfo{SegmentList{"parent-child"}},
      InterestUserModelInfo{SegmentList{"parent-child"},
                            TextEmbeddingHtmlEventList{}}};

  AdEventList ad_events;
  const AdEventInfo ad_event =
      test::BuildAdEvent(creative_ad, AdType::kNotificationAd,
                         ConfirmationType::kViewed, Now() - base::Hours(7),
                         /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const CreativeAdPredictorInputVariableInfo input_variable =
      ComputeCreativeAdPredictorInputVariable(creative_ad, user_model,
                                              ad_events);

  // Act & Assert
  EXPECT_DOUBLE_EQ(2.2958333333333329, ComputeCreativeAdPredictorScore(
                                           creative_ad, input_variable));
}

TEST_F(BraveAdsCreativeAdPredictorScoringTest,
       ComputeCreativeAdPredictorScoreForZeroDefaultWeights) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeAdModelBasedPredictorFeature,
      {{"child_intent_segment_ad_predictor_weight", "0.0"},
       {"parent_intent_segment_ad_predictor_weight", "0.0"},
       {"child_latent_interest_segment_ad_predictor_weight", "0.0"},
       {"parent_latent_interest_segment_ad_predictor_weight", "0.0"},
       {"child_interest_segment_ad_predictor_weight", "0.0"},
       {"parent_interest_segment_ad_predictor_weight", "0.0"},
       {"last_seen_ad_predictor_weight", "0.0"},
       {"last_seen_advertiser_ad_predictor_weight", "0.0"},
       {"priority_ad_predictor_weight", "0.0"}});

  CreativeAdInfo creative_ad =
      test::BuildCreativeAd(/*should_use_random_uuids=*/true);
  creative_ad.segment = "parent-child";

  const UserModelInfo user_model{
      IntentUserModelInfo{SegmentList{"parent-child"}},
      LatentInterestUserModelInfo{SegmentList{"parent-child"}},
      InterestUserModelInfo{SegmentList{"parent-child"},
                            TextEmbeddingHtmlEventList{}}};

  AdEventList ad_events;
  const AdEventInfo ad_event =
      test::BuildAdEvent(creative_ad, AdType::kNotificationAd,
                         ConfirmationType::kViewed, Now() - base::Hours(7),
                         /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const CreativeAdPredictorInputVariableInfo input_variable =
      ComputeCreativeAdPredictorInputVariable(creative_ad, user_model,
                                              ad_events);

  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, ComputeCreativeAdPredictorScore(creative_ad, input_variable));
}

}  // namespace brave_ads
