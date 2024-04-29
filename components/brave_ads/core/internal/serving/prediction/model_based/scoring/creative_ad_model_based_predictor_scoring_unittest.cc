/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/scoring/creative_ad_model_based_predictor_scoring.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/segment/creative_ad_model_based_predictor_segment_weight_unittest_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeAdModelBasedPredictorScoringTest : public UnitTestBase {};

TEST_F(BraveAdsCreativeAdModelBasedPredictorScoringTest,
       ComputeCreativeAdModelBasedPredictorScoreForDefaultWeights) {
  // Arrange
  CreativeAdInfo creative_ad =
      test::BuildCreativeAd(/*should_use_random_uuids=*/true);
  creative_ad.segment = "parent-child";

  const UserModelInfo user_model{
      IntentUserModelInfo{SegmentList{"parent-child"}},
      LatentInterestUserModelInfo{SegmentList{"parent-child"}},
      InterestUserModelInfo{SegmentList{"parent-child"}}};

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      Now() - base::Hours(7),
      /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const CreativeAdModelBasedPredictorInputVariableInfo input_variable =
      ComputeCreativeAdModelBasedPredictorInputVariable(
          creative_ad, user_model, ad_events,
          test::BuildCreativeAdModelBasedPredictorWeights());

  // Act & Assert
  EXPECT_DOUBLE_EQ(3.2916666666666665,
                   ComputeCreativeAdModelBasedPredictorScore(input_variable));
}

TEST_F(BraveAdsCreativeAdModelBasedPredictorScoringTest,
       ComputeCreativeAdModelBasedPredictorScoreForZeroDefaultWeights) {
  // Arrange
  CreativeAdInfo creative_ad =
      test::BuildCreativeAd(/*should_use_random_uuids=*/true);
  creative_ad.segment = "parent-child";

  const UserModelInfo user_model{
      IntentUserModelInfo{SegmentList{"parent-child"}},
      LatentInterestUserModelInfo{SegmentList{"parent-child"}},
      InterestUserModelInfo{SegmentList{"parent-child"}}};

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      Now() - base::Hours(7),
      /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const CreativeAdModelBasedPredictorInputVariableInfo input_variable =
      ComputeCreativeAdModelBasedPredictorInputVariable(
          creative_ad, user_model, ad_events, /*weights=*/{});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   ComputeCreativeAdModelBasedPredictorScore(input_variable));
}

}  // namespace brave_ads
