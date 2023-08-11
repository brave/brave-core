/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account_util.h"

#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAccountUtilTest : public UnitTestBase {};

TEST_F(BraveAdsAccountUtilTest, UserHasJoinedBraveRewards) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(UserHasJoinedBraveRewards());
}

TEST_F(BraveAdsAccountUtilTest, ShouldNotRewardUser) {
  // Arrange
  DisableBraveRewardsForTesting();

  // Act

  // Assert
  EXPECT_FALSE(UserHasJoinedBraveRewards());
}

TEST_F(BraveAdsAccountUtilTest, UserHasOptedInToNotificationAds) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(UserHasOptedInToNotificationAds());
}

TEST_F(BraveAdsAccountUtilTest, UserHasNotOptedInToNotificationAds) {
  // Arrange
  DisableNotificationAdsForTesting();

  // Act

  // Assert
  EXPECT_FALSE(UserHasOptedInToNotificationAds());
}

TEST_F(BraveAdsAccountUtilTest, UserHasOptedInToBraveNewsAds) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(UserHasOptedInToBraveNewsAds());
}

TEST_F(BraveAdsAccountUtilTest, UserHasNotOptedInToBraveNews) {
  // Arrange
  DisableBraveNewsAdsForTesting();

  // Act

  // Assert
  EXPECT_FALSE(UserHasOptedInToBraveNewsAds());
}

TEST_F(BraveAdsAccountUtilTest, UserHasOptedInToNewTabPageAds) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(UserHasOptedInToNewTabPageAds());
}

TEST_F(BraveAdsAccountUtilTest, UserHasNotOptedInToNewTabPageAds) {
  // Arrange
  DisableNewTabPageAdsForTesting();

  // Act

  // Assert
  EXPECT_FALSE(UserHasOptedInToNewTabPageAds());
}

}  // namespace brave_ads
