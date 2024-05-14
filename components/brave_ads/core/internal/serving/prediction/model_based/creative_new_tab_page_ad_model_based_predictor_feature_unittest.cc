/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_new_tab_page_ad_model_based_predictor_feature.h"  // IWYU pragma: keep

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(
      kCreativeNewTabPageAdModelBasedPredictorFeature));
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNewTabPageAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(
      kCreativeNewTabPageAdModelBasedPredictorFeature));
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     ChildIntentSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNewTabPageAdModelBasedPredictorFeature,
      {{"child_intent_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5, kNewTabPageAdChildIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultChildIntentSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kNewTabPageAdChildIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultChildIntentSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNewTabPageAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kNewTabPageAdChildIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     ParentIntentSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNewTabPageAdModelBasedPredictorFeature,
      {{"parent_intent_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5, kNewTabPageAdParentIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultParentIntentSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kNewTabPageAdParentIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultParentIntentSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNewTabPageAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kNewTabPageAdParentIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     ChildLatentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNewTabPageAdModelBasedPredictorFeature,
      {{"child_latent_interest_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.5, kNewTabPageAdChildLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultChildLatentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, kNewTabPageAdChildLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultChildLatentInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNewTabPageAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, kNewTabPageAdChildLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     ParentLatentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNewTabPageAdModelBasedPredictorFeature,
      {{"parent_latent_interest_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.5, kNewTabPageAdParentLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultParentLatentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, kNewTabPageAdParentLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultParentLatentInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNewTabPageAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, kNewTabPageAdParentLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     ChildInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNewTabPageAdModelBasedPredictorFeature,
      {{"child_interest_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5, kNewTabPageAdChildInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultChildInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kNewTabPageAdChildInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultChildInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNewTabPageAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kNewTabPageAdChildInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     ParentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNewTabPageAdModelBasedPredictorFeature,
      {{"parent_interest_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5,
                   kNewTabPageAdParentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultParentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kNewTabPageAdParentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultParentInterestSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNewTabPageAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kNewTabPageAdParentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     UntargetedSegmentAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNewTabPageAdModelBasedPredictorFeature,
      {{"untargeted_segment_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5, kNewTabPageAdUntargetedSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultUntargetedSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0001, kNewTabPageAdUntargetedSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultUntargetedSegmentAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNewTabPageAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0001, kNewTabPageAdUntargetedSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     LastSeenAdAdPredictorWeight) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeNewTabPageAdModelBasedPredictorFeature,
      {{"last_seen_ad_predictor_weight", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5, kNewTabPageAdLastSeenPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultLastSeenAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, kNewTabPageAdLastSeenPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     DefaultLastSeenAdPredictorWeightWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kCreativeNewTabPageAdModelBasedPredictorFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, kNewTabPageAdLastSeenPredictorWeight.Get());
}

}  // namespace brave_ads
