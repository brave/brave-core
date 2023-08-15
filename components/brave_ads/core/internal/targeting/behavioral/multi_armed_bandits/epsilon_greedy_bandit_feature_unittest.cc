/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feature.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsEpsilonGreedyBanditFeatureTest, IsEnabled) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  enabled_features.emplace_back(kEpsilonGreedyBanditFeatures, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(IsEpsilonGreedyBanditFeatureEnabled());
}

TEST(BraveAdsEpsilonGreedyBanditFeatureTest, IsDisabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(IsEpsilonGreedyBanditFeatureEnabled());
}

TEST(BraveAdsEpsilonGreedyBanditFeatureTest,
     GetEpsilonGreedyBanditEpsilonValue) {
  // Arrange
  base::FieldTrialParams params;
  params["epsilon_value"] = "0.33";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kEpsilonGreedyBanditFeatures, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(0.33, kEpsilonGreedyBanditEpsilonValue.Get());
}

TEST(BraveAdsEpsilonGreedyBanditFeatureTest,
     DefaultEpsilonGreedyBanditEpsilonValue) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(0.25, kEpsilonGreedyBanditEpsilonValue.Get());
}

TEST(BraveAdsEpsilonGreedyBanditFeatureTest,
     DefaultEpsilonGreedyBanditEpsilonValueWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kEpsilonGreedyBanditFeatures);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(0.25, kEpsilonGreedyBanditEpsilonValue.Get());
}

}  // namespace brave_ads
