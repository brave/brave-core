/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsEligibleAdsFeaturesTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsEligibleAdsEnabled());
}

TEST(BraveAdsEligibleAdsFeaturesTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kEligibleAdsFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsEligibleAdsEnabled());
}

TEST(BraveAdsEligibleAdsFeaturesTest, GetAdPredictorWeights) {
  // Arrange
  base::FieldTrialParams params;
  params["ad_predictor_weights"] = "0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kEligibleAdsFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ("0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7", kAdPredictorWeights.Get());
}

TEST(BraveAdsEligibleAdsFeaturesTest, DefaultAdFeatureWeights) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ("1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0", kAdPredictorWeights.Get());
}

TEST(BraveAdsEligibleAdsFeaturesTest, DefaultAdFeatureWeightsWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kEligibleAdsFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ("1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0", kAdPredictorWeights.Get());
}

TEST(BraveAdsEligibleAdsFeaturesTest, GetBrowsingHistoryMaxCount) {
  // Arrange
  base::FieldTrialParams params;
  params["browsing_history_max_count"] = "666";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kEligibleAdsFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(666, kBrowsingHistoryMaxCount.Get());
}

TEST(BraveAdsEligibleAdsFeaturesTest, DefaultBrowsingHistoryMaxCount) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(5'000, kBrowsingHistoryMaxCount.Get());
}

TEST(BraveAdsEligibleAdsFeaturesTest,
     DefaultBrowsingHistoryMaxCountWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kEligibleAdsFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(5'000, kBrowsingHistoryMaxCount.Get());
}

TEST(BraveAdsEligibleAdsFeaturesTest, GetBrowsingHistoryDaysAgo) {
  // Arrange
  base::FieldTrialParams params;
  params["browsing_history_days_ago"] = "7";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kEligibleAdsFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(7, kBrowsingHistoryDaysAgo.Get());
}

TEST(BraveAdsEligibleAdsFeaturesTest, DefaultBrowsingHistoryDaysAgo) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(180, kBrowsingHistoryDaysAgo.Get());
}

TEST(BraveAdsEligibleAdsFeaturesTest,
     DefaultBrowsingHistoryDaysAgoWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kEligibleAdsFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(180, kBrowsingHistoryDaysAgo.Get());
}

}  // namespace brave_ads
