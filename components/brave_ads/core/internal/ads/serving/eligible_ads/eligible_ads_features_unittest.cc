/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::features {

TEST(BatAdsEligibleAdsFeaturesTest, IsEligibleAdsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsEligibleAdsEnabled());
}

TEST(BatAdsEligibleAdsFeaturesTest, IsEligibleAdsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kEligibleAds);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsEligibleAdsEnabled());
}

TEST(BatAdsEligibleAdsFeaturesTest, GetAdPredictorWeights) {
  // Arrange
  base::FieldTrialParams params;
  params["ad_predictor_weights"] = "0.1,0.2,0.3,0.4,0.5,0.6,0.7";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kEligibleAds, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  const AdPredictorWeightList expected_ad_predictor_weights = {
      0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7};
  EXPECT_EQ(expected_ad_predictor_weights, GetAdPredictorWeights());
}

TEST(BatAdsEligibleAdsFeaturesTest, DefaultAdFeatureWeights) {
  // Arrange

  // Act

  // Assert
  const AdPredictorWeightList expected_ad_predictor_weights = {
      1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
  EXPECT_EQ(expected_ad_predictor_weights, GetAdPredictorWeights());
}

TEST(BatAdsEligibleAdsFeaturesTest, DefaultAdFeatureWeightsWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kEligibleAds);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  const AdPredictorWeightList expected_ad_predictor_weights = {
      1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
  EXPECT_EQ(expected_ad_predictor_weights, GetAdPredictorWeights());
}

TEST(BatAdsEligibleAdsFeaturesTest, GetBrowsingHistoryMaxCount) {
  // Arrange
  base::FieldTrialParams params;
  params["browsing_history_max_count"] = "666";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kEligibleAds, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(666, GetBrowsingHistoryMaxCount());
}

TEST(BatAdsEligibleAdsFeaturesTest, DefaultBrowsingHistoryMaxCount) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(5'000, GetBrowsingHistoryMaxCount());
}

TEST(BatAdsEligibleAdsFeaturesTest,
     DefaultBrowsingHistoryMaxCountWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kEligibleAds);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(5'000, GetBrowsingHistoryMaxCount());
}

TEST(BatAdsEligibleAdsFeaturesTest, GetBrowsingHistoryDaysAgo) {
  // Arrange
  base::FieldTrialParams params;
  params["browsing_history_days_ago"] = "7";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kEligibleAds, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(7, GetBrowsingHistoryDaysAgo());
}

TEST(BatAdsEligibleAdsFeaturesTest, DefaultBrowsingHistoryDaysAgo) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(180, GetBrowsingHistoryDaysAgo());
}

TEST(BatAdsEligibleAdsFeaturesTest, DefaultBrowsingHistoryDaysAgoWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kEligibleAds);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(180, GetBrowsingHistoryDaysAgo());
}

}  // namespace brave_ads::features
