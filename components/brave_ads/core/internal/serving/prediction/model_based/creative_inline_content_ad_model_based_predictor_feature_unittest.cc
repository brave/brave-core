/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_inline_content_ad_model_based_predictor_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(
      kCreativeInlineContentAdModelBasedPredictorFeature));
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeInlineContentAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(
      kCreativeInlineContentAdModelBasedPredictorFeature));
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     ChildIntentSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeInlineContentAdModelBasedPredictorFeature,
      {{"child_intent_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5,
                   kInlineContentAdChildIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultChildIntentSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kInlineContentAdChildIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultChildIntentSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeInlineContentAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kInlineContentAdChildIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     ParentIntentSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeInlineContentAdModelBasedPredictorFeature,
      {{"parent_intent_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5,
                   kInlineContentAdParentIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultParentIntentSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kInlineContentAdParentIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultParentIntentSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeInlineContentAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kInlineContentAdParentIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     ChildLatentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeInlineContentAdModelBasedPredictorFeature,
      {{"child_latent_interest_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.5, kInlineContentAdChildLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultChildLatentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, kInlineContentAdChildLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultChildLatentInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeInlineContentAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, kInlineContentAdChildLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     ParentLatentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeInlineContentAdModelBasedPredictorFeature,
      {{"parent_latent_interest_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.5, kInlineContentAdParentLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultParentLatentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, kInlineContentAdParentLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultParentLatentInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeInlineContentAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, kInlineContentAdParentLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     ChildInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeInlineContentAdModelBasedPredictorFeature,
      {{"child_interest_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5,
                   kInlineContentAdChildInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultChildInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kInlineContentAdChildInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultChildInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeInlineContentAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kInlineContentAdChildInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     ParentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeInlineContentAdModelBasedPredictorFeature,
      {{"parent_interest_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5,
                   kInlineContentAdParentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultParentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kInlineContentAdParentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultParentInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeInlineContentAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kInlineContentAdParentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     UntargetedSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeInlineContentAdModelBasedPredictorFeature,
      {{"untargeted_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5, kInlineContentAdUntargetedSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultUntargetedSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0001,
                   kInlineContentAdUntargetedSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultUntargetedSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeInlineContentAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0001,
                   kInlineContentAdUntargetedSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     LastSeenAdAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeInlineContentAdModelBasedPredictorFeature,
      {{"last_seen_ad_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5, kInlineContentAdLastSeenPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultLastSeenAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kInlineContentAdLastSeenPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     DefaultLastSeenAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeInlineContentAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kInlineContentAdLastSeenPredictorWeight.Get());
}

}  // namespace brave_ads
