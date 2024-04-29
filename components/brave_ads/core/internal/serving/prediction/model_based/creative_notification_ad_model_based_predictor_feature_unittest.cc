/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_notification_ad_model_based_predictor_feature.h"  // IWYU pragma: keep

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(
      kCreativeNotificationAdModelBasedPredictorFeature));
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNotificationAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(
      kCreativeNotificationAdModelBasedPredictorFeature));
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     ChildIntentSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNotificationAdModelBasedPredictorFeature,
      {{"child_intent_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5, kNotificationAdChildIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultChildIntentSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, kNotificationAdChildIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultChildIntentSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNotificationAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, kNotificationAdChildIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     ParentIntentSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNotificationAdModelBasedPredictorFeature,
      {{"parent_intent_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5,
                   kNotificationAdParentIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultParentIntentSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0,
                   kNotificationAdParentIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultParentIntentSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNotificationAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0,
                   kNotificationAdParentIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     ChildLatentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNotificationAdModelBasedPredictorFeature,
      {{"child_latent_interest_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.5, kNotificationAdChildLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultChildLatentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      1.0, kNotificationAdChildLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultChildLatentInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNotificationAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(
      1.0, kNotificationAdChildLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     ParentLatentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNotificationAdModelBasedPredictorFeature,
      {{"parent_latent_interest_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.5, kNotificationAdParentLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultParentLatentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      1.0, kNotificationAdParentLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultParentLatentInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNotificationAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(
      1.0, kNotificationAdParentLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     ChildInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNotificationAdModelBasedPredictorFeature,
      {{"child_interest_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5,
                   kNotificationAdChildInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultChildInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0,
                   kNotificationAdChildInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultChildInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNotificationAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0,
                   kNotificationAdChildInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     ParentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNotificationAdModelBasedPredictorFeature,
      {{"parent_interest_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5,
                   kNotificationAdParentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultParentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0,
                   kNotificationAdParentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultParentInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNotificationAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0,
                   kNotificationAdParentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     UntargetedSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNotificationAdModelBasedPredictorFeature,
      {{"untargeted_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5, kNotificationAdUntargetedSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultUntargetedSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0001,
                   kNotificationAdUntargetedSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultUntargetedSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNotificationAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0001,
                   kNotificationAdUntargetedSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     LastSeenAdAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNotificationAdModelBasedPredictorFeature,
      {{"last_seen_ad_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5, kNotificationAdLastSeenPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultLastSeenAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, kNotificationAdLastSeenPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     DefaultLastSeenAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNotificationAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, kNotificationAdLastSeenPredictorWeight.Get());
}

}  // namespace brave_ads
