/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsEpsilonGreedyBanditFeatureTest, IsEnabled) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kEpsilonGreedyBanditFeature);

  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kEpsilonGreedyBanditFeature));
}

TEST(BraveAdsEpsilonGreedyBanditFeatureTest, IsDisabled) {
  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kEpsilonGreedyBanditFeature));
}

TEST(BraveAdsEpsilonGreedyBanditFeatureTest, EpsilonGreedyBanditEpsilonValue) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kEpsilonGreedyBanditFeature, {{"epsilon_value", "0.33"}});

  // Act & Assert
  EXPECT_EQ(0.33, kEpsilonGreedyBanditEpsilonValue.Get());
}

TEST(BraveAdsEpsilonGreedyBanditFeatureTest,
     DefaultEpsilonGreedyBanditEpsilonValue) {
  // Act & Assert
  EXPECT_EQ(0.25, kEpsilonGreedyBanditEpsilonValue.Get());
}

TEST(BraveAdsEpsilonGreedyBanditFeatureTest,
     DefaultEpsilonGreedyBanditEpsilonValueWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kEpsilonGreedyBanditFeature);

  // Act & Assert
  EXPECT_EQ(0.25, kEpsilonGreedyBanditEpsilonValue.Get());
}

}  // namespace brave_ads
