/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/epsilon_greedy_bandit_features.h"

#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::targeting::features {

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

}  // namespace ads::targeting::features
