/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(
      base::FeatureList::IsEnabled(kCreativeAdModelBasedPredictorFeature));
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeAdModelBasedPredictorFeature);

  // Act

  // Assert
  EXPECT_FALSE(
      base::FeatureList::IsEnabled(kCreativeAdModelBasedPredictorFeature));
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     ChildIntentSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeAdModelBasedPredictorFeature,
      {{"child_intent_segment_ad_predictor_weight", "0.5"}});

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(0.5, kChildIntentSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultChildIntentSegmentAdPredictorWeight) {
  // Arrange

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kChildIntentSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultChildIntentSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeAdModelBasedPredictorFeature);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kChildIntentSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     ParentIntentSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeAdModelBasedPredictorFeature,
      {{"parent_intent_segment_ad_predictor_weight", "0.5"}});

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(0.5, kParentIntentSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultParentIntentSegmentAdPredictorWeight) {
  // Arrange

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kParentIntentSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultParentIntentSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeAdModelBasedPredictorFeature);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kParentIntentSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     ChildLatentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeAdModelBasedPredictorFeature,
      {{"child_latent_interest_segment_ad_predictor_weight", "0.5"}});

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(0.5, kChildLatentInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultChildLatentInterestSegmentAdPredictorWeight) {
  // Arrange

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kChildLatentInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultChildLatentInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeAdModelBasedPredictorFeature);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kChildLatentInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     ParentLatentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeAdModelBasedPredictorFeature,
      {{"parent_latent_interest_segment_ad_predictor_weight", "0.5"}});

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(0.5, kParentLatentInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultParentLatentInterestSegmentAdPredictorWeight) {
  // Arrange

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kParentLatentInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultParentLatentInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeAdModelBasedPredictorFeature);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kParentLatentInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     ChildInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeAdModelBasedPredictorFeature,
      {{"child_interest_segment_ad_predictor_weight", "0.5"}});

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(0.5, kChildInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultChildInterestSegmentAdPredictorWeight) {
  // Arrange

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kChildInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultChildInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeAdModelBasedPredictorFeature);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kChildInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     ParentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeAdModelBasedPredictorFeature,
      {{"parent_interest_segment_ad_predictor_weight", "0.5"}});

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(0.5, kParentInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultParentInterestSegmentAdPredictorWeight) {
  // Arrange

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kParentInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultParentInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeAdModelBasedPredictorFeature);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kParentInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     LastSeenAdAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeAdModelBasedPredictorFeature,
      {{"last_seen_ad_predictor_weight", "0.5"}});

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(0.5, kLastSeenAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultLastSeenAdPredictorWeight) {
  // Arrange

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kLastSeenAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultLastSeenAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeAdModelBasedPredictorFeature);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kLastSeenAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     LastSeenAdvertiserAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeAdModelBasedPredictorFeature,
      {{"last_seen_advertiser_ad_predictor_weight", "0.5"}});

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(0.5, kLastSeenAdvertiserAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultLastSeenAdvertiserAdPredictorWeight) {
  // Arrange

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kLastSeenAdvertiserAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultLastSeenAdvertiserAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeAdModelBasedPredictorFeature);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kLastSeenAdvertiserAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     PriorityAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeAdModelBasedPredictorFeature,
      {{"priority_ad_predictor_weight", "0.5"}});

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(0.5, kPriorityAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultPriorityAdPredictorWeight) {
  // Arrange

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kPriorityAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     DefaultPriorityAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeAdModelBasedPredictorFeature);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kPriorityAdPredictorWeight.Get());
}

}  // namespace brave_ads
