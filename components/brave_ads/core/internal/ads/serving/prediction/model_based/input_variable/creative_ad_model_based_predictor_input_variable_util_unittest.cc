/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/prediction/model_based/input_variable/creative_ad_model_based_predictor_input_variable_util.h"

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeAdPredictorInputVariableUtilTest : public UnitTestBase {};

TEST_F(BraveAdsCreativeAdPredictorInputVariableUtilTest,
       ComputeCreativeAdPredictorMatchingChildIntentSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.intent_segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdPredictorSegmentInputVariableInfo input_variable =
      ComputeCreativeAdPredictorIntentSegmentInputVariable(user_model,
                                                           "parent-child");

  // Assert
  EXPECT_TRUE(input_variable.does_match_child);
  EXPECT_TRUE(input_variable.does_match_parent);
}

TEST_F(BraveAdsCreativeAdPredictorInputVariableUtilTest,
       ComputeCreativeAdPredictorMatchingParentIntentSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.intent_segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdPredictorSegmentInputVariableInfo input_variable =
      ComputeCreativeAdPredictorIntentSegmentInputVariable(user_model,
                                                           "parent-foo");

  // Assert
  EXPECT_FALSE(input_variable.does_match_child);
  EXPECT_TRUE(input_variable.does_match_parent);
}

TEST_F(BraveAdsCreativeAdPredictorInputVariableUtilTest,
       ComputeCreativeAdPredictorNonMatchingIntentSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.intent_segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdPredictorSegmentInputVariableInfo input_variable =
      ComputeCreativeAdPredictorIntentSegmentInputVariable(user_model,
                                                           "foo-bar");

  // Assert
  EXPECT_FALSE(input_variable.does_match_child);
  EXPECT_FALSE(input_variable.does_match_parent);
}

TEST_F(
    BraveAdsCreativeAdPredictorInputVariableUtilTest,
    ComputeCreativeAdPredictorMatchingChildLatentInterestSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.latent_interest_segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdPredictorSegmentInputVariableInfo input_variable =
      ComputeCreativeAdPredictorLatentInterestSegmentInputVariable(
          user_model, "parent-child");

  // Assert
  EXPECT_TRUE(input_variable.does_match_child);
  EXPECT_TRUE(input_variable.does_match_parent);
}

TEST_F(
    BraveAdsCreativeAdPredictorInputVariableUtilTest,
    ComputeCreativeAdPredictorMatchingParentLatentInterestSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.latent_interest_segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdPredictorSegmentInputVariableInfo input_variable =
      ComputeCreativeAdPredictorLatentInterestSegmentInputVariable(
          user_model, "parent-foo");

  // Assert
  EXPECT_FALSE(input_variable.does_match_child);
  EXPECT_TRUE(input_variable.does_match_parent);
}

TEST_F(
    BraveAdsCreativeAdPredictorInputVariableUtilTest,
    ComputeCreativeAdPredictorNonMatchingLatentInterestSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.latent_interest_segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdPredictorSegmentInputVariableInfo input_variable =
      ComputeCreativeAdPredictorLatentInterestSegmentInputVariable(user_model,
                                                                   "foo-bar");

  // Assert
  EXPECT_FALSE(input_variable.does_match_child);
  EXPECT_FALSE(input_variable.does_match_parent);
}

TEST_F(BraveAdsCreativeAdPredictorInputVariableUtilTest,
       ComputeCreativeAdPredictorMatchingChildInterestSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.interest_segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdPredictorSegmentInputVariableInfo input_variable =
      ComputeCreativeAdPredictorInterestSegmentInputVariable(user_model,
                                                             "parent-child");

  // Assert
  EXPECT_TRUE(input_variable.does_match_child);
  EXPECT_TRUE(input_variable.does_match_parent);
}

TEST_F(BraveAdsCreativeAdPredictorInputVariableUtilTest,
       ComputeCreativeAdPredictorMatchingParentInterestSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.interest_segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdPredictorSegmentInputVariableInfo input_variable =
      ComputeCreativeAdPredictorInterestSegmentInputVariable(user_model,
                                                             "parent-foo");

  // Assert
  EXPECT_FALSE(input_variable.does_match_child);
  EXPECT_TRUE(input_variable.does_match_parent);
}

TEST_F(BraveAdsCreativeAdPredictorInputVariableUtilTest,
       ComputeCreativeAdPredictorNonMatchingInterestSegmentInputVariable) {
  // Arrange
  UserModelInfo user_model;
  user_model.interest_segments = {"parent-child", "xyzzy-thud"};

  // Act
  const CreativeAdPredictorSegmentInputVariableInfo input_variable =
      ComputeCreativeAdPredictorInterestSegmentInputVariable(user_model,
                                                             "foo-bar");

  // Assert
  EXPECT_FALSE(input_variable.does_match_child);
  EXPECT_FALSE(input_variable.does_match_parent);
}

TEST_F(BraveAdsCreativeAdPredictorInputVariableUtilTest,
       ComputeCreativeAdPredictorLastSeenAdInputVariable) {
  // Arrange
  const CreativeAdInfo creative_ad =
      BuildCreativeAdForTesting(/*should_use_random_uuids*/ true);

  AdEventList ad_events;
  const AdEventInfo ad_event =
      BuildAdEventForTesting(creative_ad, AdType::kNotificationAd,
                             ConfirmationType::kViewed, Now() - base::Hours(7));
  ad_events.push_back(ad_event);

  // Act
  const absl::optional<base::TimeDelta> input_variable =
      ComputeCreativeAdPredictorLastSeenAdInputVariable(creative_ad, ad_events);

  // Assert
  EXPECT_EQ(base::Hours(7), input_variable);
}

TEST_F(BraveAdsCreativeAdPredictorInputVariableUtilTest,
       ComputeCreativeAdPredictorLastSeenAdInputVariableIfNeverSeen) {
  // Arrange
  const CreativeAdInfo creative_ad =
      BuildCreativeAdForTesting(/*should_use_random_uuids*/ true);

  const AdEventList ad_events;

  // Act
  const absl::optional<base::TimeDelta> input_variable =
      ComputeCreativeAdPredictorLastSeenAdInputVariable(creative_ad, ad_events);

  // Assert
  EXPECT_FALSE(input_variable);
}

TEST_F(BraveAdsCreativeAdPredictorInputVariableUtilTest,
       ComputeCreativeAdPredictorLastSeenAdvertiserInputVariable) {
  // Arrange
  const CreativeAdInfo creative_ad =
      BuildCreativeAdForTesting(/*should_use_random_uuids*/ true);

  AdEventList ad_events;
  const AdEventInfo ad_event =
      BuildAdEventForTesting(creative_ad, AdType::kNotificationAd,
                             ConfirmationType::kViewed, Now() - base::Hours(3));
  ad_events.push_back(ad_event);

  // Act
  const absl::optional<base::TimeDelta> input_variable =
      ComputeCreativeAdPredictorLastSeenAdvertiserInputVariable(creative_ad,
                                                                ad_events);

  // Assert
  EXPECT_EQ(base::Hours(3), input_variable);
}

TEST_F(BraveAdsCreativeAdPredictorInputVariableUtilTest,
       ComputeCreativeAdPredictorLastSeenAdvertiserInputVariableIfNeverSeen) {
  // Arrange
  const CreativeAdInfo creative_ad =
      BuildCreativeAdForTesting(/*should_use_random_uuids*/ true);

  const AdEventList ad_events;

  // Act
  const absl::optional<base::TimeDelta> input_variable =
      ComputeCreativeAdPredictorLastSeenAdvertiserInputVariable(creative_ad,
                                                                ad_events);

  // Assert
  EXPECT_FALSE(input_variable);
}

}  // namespace brave_ads
