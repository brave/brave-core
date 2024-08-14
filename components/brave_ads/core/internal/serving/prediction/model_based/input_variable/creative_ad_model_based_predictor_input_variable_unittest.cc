/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/segment/creative_ad_model_based_predictor_segment_weight_test_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder_test_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeAdModelBasedPredictorInputVariableTest
    : public test::TestBase {};

TEST_F(BraveAdsCreativeAdModelBasedPredictorInputVariableTest,
       ComputeCreativeAdModelBasedPredictorInputVariable) {
  // Arrange
  CreativeAdInfo creative_ad =
      test::BuildCreativeAd(/*should_generate_random_uuids=*/true);
  creative_ad.segment = "parent-child";

  const UserModelInfo user_model{
      IntentUserModelInfo{SegmentList{"parent-child"}},
      LatentInterestUserModelInfo{SegmentList{"xyzzy-thud"}},
      InterestUserModelInfo{SegmentList{"parent"}}};

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      test::Now() - base::Hours(7),
      /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event);

  // Act
  const CreativeAdModelBasedPredictorInputVariableInfo input_variable =
      ComputeCreativeAdModelBasedPredictorInputVariable(
          creative_ad, user_model, ad_events,
          test::BuildCreativeAdModelBasedPredictorWeights());

  // Assert
  CreativeAdModelBasedPredictorInputVariableInfo expected_input_variable;
  expected_input_variable.intent_segment.child_matches.value = true;
  expected_input_variable.intent_segment.parent_matches.value = true;
  expected_input_variable.latent_interest_segment.child_matches.value = false;
  expected_input_variable.latent_interest_segment.parent_matches.value = false;
  expected_input_variable.interest_segment.child_matches.value = false;
  expected_input_variable.interest_segment.parent_matches.value = true;
  expected_input_variable.untargeted_segment.value = false;
  expected_input_variable.last_seen_ad.value = base::Hours(7);
  EXPECT_EQ(expected_input_variable, input_variable);
}

}  // namespace brave_ads
