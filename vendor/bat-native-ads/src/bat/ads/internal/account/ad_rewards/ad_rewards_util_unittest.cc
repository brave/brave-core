/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/ad_rewards/ad_rewards_util.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAdRewardsUtilTest : public UnitTestBase {
 protected:
  BatAdsAdRewardsUtilTest() = default;

  ~BatAdsAdRewardsUtilTest() override = default;
};

TEST_F(BatAdsAdRewardsUtilTest, ShouldRewardUser) {
  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, true);

  // Act
  const bool should_reward_user = ShouldRewardUser();

  // Assert
  EXPECT_TRUE(should_reward_user);
}

TEST_F(BatAdsAdRewardsUtilTest, ShouldNotRewardUser) {
  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, false);

  // Act
  const bool should_reward_user = ShouldRewardUser();

  // Assert
  EXPECT_FALSE(should_reward_user);
}

}  // namespace ads
