/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor_info.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder_test_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeAdModelBasedPredictorUtilTest : public test::TestBase {};

TEST_F(BraveAdsCreativeAdModelBasedPredictorUtilTest,
       ComputeCreativeAdModelBasedPredictors) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/
                                        true);
  creative_ad_1.segment = "parent-child";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/
                                        true);
  creative_ad_2.segment = "xyzzy-thud";
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/
                                        true);
  creative_ad_3.segment = "parent";
  creative_ads.push_back(creative_ad_3);

  const UserModelInfo user_model{
      IntentUserModelInfo{SegmentList{"parent-child"}},
      LatentInterestUserModelInfo{SegmentList{"foo-bar"}},
      InterestUserModelInfo{SegmentList{"parent"}}};

  AdEventList ad_events;
  const AdEventInfo ad_event =
      test::BuildAdEvent(creative_ad_2, AdType::kNotificationAd,
                         ConfirmationType::kViewedImpression,
                         /*created_at=*/test::Now() - base::Hours(3),
                         /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event);

  // Act
  const CreativeAdModelBasedPredictorList<CreativeNotificationAdInfo>
      creative_ad_predictors = ComputeCreativeAdModelBasedPredictors(
          creative_ads, user_model, ad_events);

  // Assert
  CreativeAdModelBasedPredictorList<CreativeNotificationAdInfo>
      expected_creative_ad_predictors;

  CreativeAdModelBasedPredictorInfo<CreativeNotificationAdInfo>
      expected_creative_ad_predictor_1;
  expected_creative_ad_predictor_1.creative_ad = creative_ad_1;
  expected_creative_ad_predictor_1.input_variable.intent_segment.child_matches
      .value = true;
  expected_creative_ad_predictor_1.input_variable.intent_segment.parent_matches
      .value = true;
  expected_creative_ad_predictor_1.input_variable.latent_interest_segment
      .child_matches.value = false;
  expected_creative_ad_predictor_1.input_variable.latent_interest_segment
      .parent_matches.value = false;
  expected_creative_ad_predictor_1.input_variable.interest_segment.child_matches
      .value = false;
  expected_creative_ad_predictor_1.input_variable.interest_segment
      .parent_matches.value = true;
  expected_creative_ad_predictor_1.score = 2.0;
  expected_creative_ad_predictors.push_back(expected_creative_ad_predictor_1);

  CreativeAdModelBasedPredictorInfo<CreativeNotificationAdInfo>
      expected_creative_ad_predictor_2;
  expected_creative_ad_predictor_2.creative_ad = creative_ad_2;
  expected_creative_ad_predictor_2.input_variable.intent_segment.child_matches
      .value = false;
  expected_creative_ad_predictor_2.input_variable.intent_segment.parent_matches
      .value = false;
  expected_creative_ad_predictor_2.input_variable.latent_interest_segment
      .child_matches.value = false;
  expected_creative_ad_predictor_2.input_variable.latent_interest_segment
      .parent_matches.value = false;
  expected_creative_ad_predictor_2.input_variable.interest_segment.child_matches
      .value = false;
  expected_creative_ad_predictor_2.input_variable.interest_segment
      .parent_matches.value = false;
  expected_creative_ad_predictor_2.input_variable.last_seen_ad.value =
      base::Hours(3);
  expected_creative_ad_predictor_2.score = 0.0;
  expected_creative_ad_predictors.push_back(expected_creative_ad_predictor_2);

  CreativeAdModelBasedPredictorInfo<CreativeNotificationAdInfo>
      expected_creative_ad_predictor_3;
  expected_creative_ad_predictor_3.creative_ad = creative_ad_3;
  expected_creative_ad_predictor_3.input_variable.intent_segment.child_matches
      .value = false;
  expected_creative_ad_predictor_3.input_variable.intent_segment.parent_matches
      .value = true;
  expected_creative_ad_predictor_3.input_variable.latent_interest_segment
      .child_matches.value = false;
  expected_creative_ad_predictor_3.input_variable.latent_interest_segment
      .parent_matches.value = false;
  expected_creative_ad_predictor_3.input_variable.interest_segment.child_matches
      .value = true;
  expected_creative_ad_predictor_3.input_variable.interest_segment
      .parent_matches.value = true;
  expected_creative_ad_predictor_3.score = 2.0;
  expected_creative_ad_predictors.push_back(expected_creative_ad_predictor_3);

  EXPECT_EQ(expected_creative_ad_predictors, creative_ad_predictors);
}

}  // namespace brave_ads
