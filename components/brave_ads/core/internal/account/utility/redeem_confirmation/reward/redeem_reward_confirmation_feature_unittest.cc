/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/redeem_reward_confirmation_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsRedeemRewardConfirmationFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kRedeemRewardConfirmationFeature));
}

TEST(BraveAdsRedeemRewardConfirmationFeatureTest, FetchPaymentTokenAfter) {
  // Act & Assert
  EXPECT_EQ(base::Seconds(3), kFetchPaymentTokenAfter.Get());
}

}  // namespace brave_ads
