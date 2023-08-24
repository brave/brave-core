/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/prediction/model_based/creative_ad_model_based_predictor_feature.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(
      base::FeatureList::IsEnabled(CreativeAdModelBasedPredictorFeature));
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(CreativeAdModelBasedPredictorFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(
      base::FeatureList::IsEnabled(CreativeAdModelBasedPredictorFeature));
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     ChildIntentSegmentAdPredictorWeight) {
  // Arrange
  base::FieldTrialParams params;
  params["child_intent_segment_ad_predictor_weight"] = "0.5";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(CreativeAdModelBasedPredictorFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

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
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(CreativeAdModelBasedPredictorFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kChildIntentSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     ParentIntentSegmentAdPredictorWeight) {
  // Arrange
  base::FieldTrialParams params;
  params["parent_intent_segment_ad_predictor_weight"] = "0.5";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(CreativeAdModelBasedPredictorFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

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
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(CreativeAdModelBasedPredictorFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kParentIntentSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     ChildLatentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::FieldTrialParams params;
  params["child_latent_interest_segment_ad_predictor_weight"] = "0.5";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(CreativeAdModelBasedPredictorFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

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
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(CreativeAdModelBasedPredictorFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kChildLatentInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     ParentLatentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::FieldTrialParams params;
  params["parent_latent_interest_segment_ad_predictor_weight"] = "0.5";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(CreativeAdModelBasedPredictorFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

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
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(CreativeAdModelBasedPredictorFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kParentLatentInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     ChildInterestSegmentAdPredictorWeight) {
  // Arrange
  base::FieldTrialParams params;
  params["child_interest_segment_ad_predictor_weight"] = "0.5";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(CreativeAdModelBasedPredictorFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

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
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(CreativeAdModelBasedPredictorFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kChildInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     ParentInterestSegmentAdPredictorWeight) {
  // Arrange
  base::FieldTrialParams params;
  params["parent_interest_segment_ad_predictor_weight"] = "0.5";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(CreativeAdModelBasedPredictorFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

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
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(CreativeAdModelBasedPredictorFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kParentInterestSegmentAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     LastSeenAdAdPredictorWeight) {
  // Arrange
  base::FieldTrialParams params;
  params["last_seen_ad_predictor_weight"] = "0.5";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(CreativeAdModelBasedPredictorFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

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
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(CreativeAdModelBasedPredictorFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kLastSeenAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     LastSeenAdvertiserAdPredictorWeight) {
  // Arrange
  base::FieldTrialParams params;
  params["last_seen_advertiser_ad_predictor_weight"] = "0.5";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(CreativeAdModelBasedPredictorFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

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
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(CreativeAdModelBasedPredictorFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kLastSeenAdvertiserAdPredictorWeight.Get());
}

TEST(BraveAdsCreativeAdModelBasedPredictorFeatureTest,
     PriorityAdPredictorWeight) {
  // Arrange
  base::FieldTrialParams params;
  params["priority_ad_predictor_weight"] = "0.5";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(CreativeAdModelBasedPredictorFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

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
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(CreativeAdModelBasedPredictorFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(1.0, kPriorityAdPredictorWeight.Get());
}

}  // namespace brave_ads
