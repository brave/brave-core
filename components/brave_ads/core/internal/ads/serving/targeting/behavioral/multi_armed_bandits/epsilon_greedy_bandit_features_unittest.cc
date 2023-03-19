/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_features.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::targeting::features {

TEST(BatAdsEpsilonGreedyBanditFeaturesTest, EpsilonGreedyBanditEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(IsEpsilonGreedyBanditEnabled());
}

TEST(BatAdsEpsilonGreedyBanditFeaturesTest, EpsilonGreedyBanditEpsilon) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(0.25, GetEpsilonGreedyBanditEpsilonValue());
}

}  // namespace brave_ads::targeting::features
