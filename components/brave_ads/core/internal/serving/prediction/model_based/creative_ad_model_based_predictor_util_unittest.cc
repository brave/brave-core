/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeAdModelBasedPredictorUtilTest : public UnitTestBase {};

TEST_F(BraveAdsCreativeAdModelBasedPredictorUtilTest,
       ComputeCreativeAdPredictors) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/
                                        true);
  creative_ad_1.segment = "parent-child";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/
                                        true);
  creative_ad_2.segment = "xyzzy-thud";
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/
                                        true);
  creative_ad_3.segment = "parent";
  creative_ads.push_back(creative_ad_3);

  const UserModelInfo user_model{
      IntentUserModelInfo{SegmentList{"parent-child"}},
      LatentInterestUserModelInfo{SegmentList{"foo-bar"}},
      InterestUserModelInfo{SegmentList{"parent"},
                            TextEmbeddingHtmlEventList{}}};

  AdEventList ad_events;
  const AdEventInfo ad_event =
      test::BuildAdEvent(creative_ad_2, AdType::kNotificationAd,
                         ConfirmationType::kViewed, Now() - base::Hours(3),
                         /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event);

  // Act & Assert
  CreativeAdPredictorList<CreativeNotificationAdInfo>
      expected_creative_ad_predictors;

  CreativeAdPredictorInfo<CreativeNotificationAdInfo>
      expected_creative_ad_predictor_1;
  expected_creative_ad_predictor_1.creative_ad = creative_ad_1;
  expected_creative_ad_predictor_1.input_variable.intent_segment
      .does_match_child = true;
  expected_creative_ad_predictor_1.input_variable.intent_segment
      .does_match_parent = true;
  expected_creative_ad_predictor_1.input_variable.latent_interest_segment
      .does_match_child = false;
  expected_creative_ad_predictor_1.input_variable.latent_interest_segment
      .does_match_parent = false;
  expected_creative_ad_predictor_1.input_variable.interest_segment
      .does_match_child = false;
  expected_creative_ad_predictor_1.input_variable.interest_segment
      .does_match_parent = true;
  expected_creative_ad_predictor_1.score = 4.5;
  expected_creative_ad_predictors.push_back(expected_creative_ad_predictor_1);

  CreativeAdPredictorInfo<CreativeNotificationAdInfo>
      expected_creative_ad_predictor_2;
  expected_creative_ad_predictor_2.creative_ad = creative_ad_2;
  expected_creative_ad_predictor_2.input_variable.intent_segment
      .does_match_child = false;
  expected_creative_ad_predictor_2.input_variable.intent_segment
      .does_match_parent = false;
  expected_creative_ad_predictor_2.input_variable.latent_interest_segment
      .does_match_child = false;
  expected_creative_ad_predictor_2.input_variable.latent_interest_segment
      .does_match_parent = false;
  expected_creative_ad_predictor_2.input_variable.interest_segment
      .does_match_child = false;
  expected_creative_ad_predictor_2.input_variable.interest_segment
      .does_match_parent = false;
  expected_creative_ad_predictor_2.input_variable.last_seen_ad = base::Hours(3);
  expected_creative_ad_predictor_2.input_variable.last_seen_advertiser =
      base::Hours(3);
  expected_creative_ad_predictor_2.score = 0.75;
  expected_creative_ad_predictors.push_back(expected_creative_ad_predictor_2);

  CreativeAdPredictorInfo<CreativeNotificationAdInfo>
      expected_creative_ad_predictor_3;
  expected_creative_ad_predictor_3.creative_ad = creative_ad_3;
  expected_creative_ad_predictor_3.input_variable.intent_segment
      .does_match_child = false;
  expected_creative_ad_predictor_3.input_variable.intent_segment
      .does_match_parent = true;
  expected_creative_ad_predictor_3.input_variable.latent_interest_segment
      .does_match_child = false;
  expected_creative_ad_predictor_3.input_variable.latent_interest_segment
      .does_match_parent = false;
  expected_creative_ad_predictor_3.input_variable.interest_segment
      .does_match_child = true;
  expected_creative_ad_predictor_3.input_variable.interest_segment
      .does_match_parent = true;
  expected_creative_ad_predictor_3.score = 4.5;
  expected_creative_ad_predictors.push_back(expected_creative_ad_predictor_3);

  EXPECT_EQ(expected_creative_ad_predictors,
            ComputeCreativeAdPredictors(creative_ads, user_model, ad_events));
}

}  // namespace brave_ads
