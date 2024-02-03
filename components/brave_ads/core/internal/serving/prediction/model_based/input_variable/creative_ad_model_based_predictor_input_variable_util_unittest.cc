/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable_util.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/input_variable/segment/creative_ad_model_based_predictor_segment_input_variables_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeAdModelBasedPredictorInputVariableUtilTest
    : public UnitTestBase {};

TEST_F(
    BraveAdsCreativeAdModelBasedPredictorInputVariableUtilTest,
    ComputeCreativeAdModelBasedPredictorMatchingChildIntentSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.intent.segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdModelBasedPredictorSegmentInputVariablesInfo
      intent_segment_input_variable =
          ComputeCreativeAdModelBasedPredictorIntentSegmentInputVariable(
              user_model, "parent-child", /*weights=*/{});

  // Assert
  EXPECT_TRUE(intent_segment_input_variable.child_matches.value);
  EXPECT_TRUE(intent_segment_input_variable.parent_matches.value);
}

TEST_F(
    BraveAdsCreativeAdModelBasedPredictorInputVariableUtilTest,
    ComputeCreativeAdModelBasedPredictorMatchingParentIntentSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.intent.segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdModelBasedPredictorSegmentInputVariablesInfo
      intent_segment_input_variable =
          ComputeCreativeAdModelBasedPredictorIntentSegmentInputVariable(
              user_model, "parent-foo", /*weights=*/{});

  // Assert
  EXPECT_FALSE(intent_segment_input_variable.child_matches.value);
  EXPECT_TRUE(intent_segment_input_variable.parent_matches.value);
}

TEST_F(
    BraveAdsCreativeAdModelBasedPredictorInputVariableUtilTest,
    ComputeCreativeAdModelBasedPredictorNonMatchingIntentSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.intent.segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdModelBasedPredictorSegmentInputVariablesInfo
      intent_segment_input_variable =
          ComputeCreativeAdModelBasedPredictorIntentSegmentInputVariable(
              user_model, "foo-bar", /*weights=*/{});

  // Assert
  EXPECT_FALSE(intent_segment_input_variable.child_matches.value);
  EXPECT_FALSE(intent_segment_input_variable.parent_matches.value);
}

TEST_F(
    BraveAdsCreativeAdModelBasedPredictorInputVariableUtilTest,
    ComputeCreativeAdModelBasedPredictorMatchingChildLatentInterestSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.latent_interest.segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdModelBasedPredictorSegmentInputVariablesInfo
      latent_interest_segment_input_variable =
          ComputeCreativeAdModelBasedPredictorLatentInterestSegmentInputVariable(
              user_model, "parent-child", /*weights=*/{});

  // Assert
  EXPECT_TRUE(latent_interest_segment_input_variable.child_matches.value);
  EXPECT_TRUE(latent_interest_segment_input_variable.parent_matches.value);
}

TEST_F(
    BraveAdsCreativeAdModelBasedPredictorInputVariableUtilTest,
    ComputeCreativeAdModelBasedPredictorMatchingParentLatentInterestSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.latent_interest.segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdModelBasedPredictorSegmentInputVariablesInfo
      latent_interest_segment_input_variable =
          ComputeCreativeAdModelBasedPredictorLatentInterestSegmentInputVariable(
              user_model, "parent-foo", /*weights=*/{});

  // Assert
  EXPECT_FALSE(latent_interest_segment_input_variable.child_matches.value);
  EXPECT_TRUE(latent_interest_segment_input_variable.parent_matches.value);
}

TEST_F(
    BraveAdsCreativeAdModelBasedPredictorInputVariableUtilTest,
    ComputeCreativeAdModelBasedPredictorNonMatchingLatentInterestSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.latent_interest.segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdModelBasedPredictorSegmentInputVariablesInfo
      latent_interest_segment_input_variable =
          ComputeCreativeAdModelBasedPredictorLatentInterestSegmentInputVariable(
              user_model, "foo-bar", /*weights=*/{});

  // Assert
  EXPECT_FALSE(latent_interest_segment_input_variable.child_matches.value);
  EXPECT_FALSE(latent_interest_segment_input_variable.parent_matches.value);
}

TEST_F(
    BraveAdsCreativeAdModelBasedPredictorInputVariableUtilTest,
    ComputeCreativeAdModelBasedPredictorMatchingChildInterestSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.interest.segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdModelBasedPredictorSegmentInputVariablesInfo
      interest_segment_input_variable =
          ComputeCreativeAdModelBasedPredictorInterestSegmentInputVariable(
              user_model, "parent-child", /*weights=*/{});

  // Assert
  EXPECT_TRUE(interest_segment_input_variable.child_matches.value);
  EXPECT_TRUE(interest_segment_input_variable.parent_matches.value);
}

TEST_F(
    BraveAdsCreativeAdModelBasedPredictorInputVariableUtilTest,
    ComputeCreativeAdModelBasedPredictorMatchingParentInterestSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.interest.segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdModelBasedPredictorSegmentInputVariablesInfo
      interest_segment_input_variable =
          ComputeCreativeAdModelBasedPredictorInterestSegmentInputVariable(
              user_model, "parent-foo", /*weights=*/{});

  // Assert
  EXPECT_FALSE(interest_segment_input_variable.child_matches.value);
  EXPECT_TRUE(interest_segment_input_variable.parent_matches.value);
}

TEST_F(
    BraveAdsCreativeAdModelBasedPredictorInputVariableUtilTest,
    ComputeCreativeAdModelBasedPredictorNonMatchingInterestSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.interest.segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdModelBasedPredictorSegmentInputVariablesInfo
      interest_segment_input_variable =
          ComputeCreativeAdModelBasedPredictorInterestSegmentInputVariable(
              user_model, "foo-bar", /*weights=*/{});

  // Assert
  EXPECT_FALSE(interest_segment_input_variable.child_matches.value);
  EXPECT_FALSE(interest_segment_input_variable.parent_matches.value);
}

TEST_F(BraveAdsCreativeAdModelBasedPredictorInputVariableUtilTest,
       ComputeCreativeAdModelBasedPredictorLastSeenAdInputVariable) {
  // Arrange
  const CreativeAdInfo creative_ad =
      test::BuildCreativeAd(/*should_use_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event =
      test::BuildAdEvent(creative_ad, AdType::kNotificationAd,
                         ConfirmationType::kViewed, Now() - base::Hours(7),
                         /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const CreativeAdModelBasedPredictorLastSeenInputVariableInfo
      last_seen_ad_input_variable =
          ComputeCreativeAdModelBasedPredictorLastSeenAdInputVariable(
              creative_ad, ad_events, /*weight=*/0.0);

  // Act & Assert
  EXPECT_EQ(base::Hours(7), last_seen_ad_input_variable.value);
}

TEST_F(BraveAdsCreativeAdModelBasedPredictorInputVariableUtilTest,
       ComputeCreativeAdModelBasedPredictorLastSeenAdInputVariableIfNeverSeen) {
  // Arrange
  const CreativeAdInfo creative_ad =
      test::BuildCreativeAd(/*should_use_random_uuids=*/true);

  const CreativeAdModelBasedPredictorLastSeenInputVariableInfo
      last_seen_ad_input_variable =
          ComputeCreativeAdModelBasedPredictorLastSeenAdInputVariable(
              creative_ad, /*ad_events=*/{}, /*weight=*/0.0);

  // Act & Assert
  EXPECT_FALSE(last_seen_ad_input_variable.value);
}

TEST_F(BraveAdsCreativeAdModelBasedPredictorInputVariableUtilTest,
       ComputeCreativeAdModelBasedPredictorLastSeenAdvertiserInputVariable) {
  // Arrange
  const CreativeAdInfo creative_ad =
      test::BuildCreativeAd(/*should_use_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event =
      test::BuildAdEvent(creative_ad, AdType::kNotificationAd,
                         ConfirmationType::kViewed, Now() - base::Hours(3),
                         /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const CreativeAdModelBasedPredictorLastSeenInputVariableInfo
      last_seen_advertiser_input_variable =
          ComputeCreativeAdModelBasedPredictorLastSeenAdvertiserInputVariable(
              creative_ad, ad_events, /*weight=*/0.0);

  // Act & Assert
  EXPECT_EQ(base::Hours(3), last_seen_advertiser_input_variable.value);
}

TEST_F(
    BraveAdsCreativeAdModelBasedPredictorInputVariableUtilTest,
    ComputeCreativeAdModelBasedPredictorLastSeenAdvertiserInputVariableIfNeverSeen) {
  // Arrange
  const CreativeAdInfo creative_ad =
      test::BuildCreativeAd(/*should_use_random_uuids=*/true);

  const CreativeAdModelBasedPredictorLastSeenInputVariableInfo
      last_seen_advertiser_input_variable =
          ComputeCreativeAdModelBasedPredictorLastSeenAdvertiserInputVariable(
              creative_ad, /*ad_events=*/{}, /*weight=*/0.0);

  // Act & Assert
  EXPECT_FALSE(last_seen_advertiser_input_variable.value);
}

}  // namespace brave_ads
