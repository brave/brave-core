/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/redeem_reward_confirmation_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsRedeemRewardConfirmationFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kRedeemRewardConfirmationFeature));
}

TEST(BraveAdsRedeemRewardConfirmationFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kRedeemRewardConfirmationFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kRedeemRewardConfirmationFeature));
}

TEST(BraveAdsRedeemRewardConfirmationFeatureTest, FetchPaymentTokenAfter) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kRedeemRewardConfirmationFeature, {{"fetch_payment_token_after", "5s"}});

  // Act & Assert
  EXPECT_EQ(base::Seconds(5), kFetchPaymentTokenAfter.Get());
}

TEST(BraveAdsRedeemRewardConfirmationFeatureTest,
     DefaultFetchPaymentTokenAfter) {
  // Act & Assert
  EXPECT_EQ(base::Seconds(15), kFetchPaymentTokenAfter.Get());
}

TEST(BraveAdsRedeemRewardConfirmationFeatureTest,
     DefaultProcessConversionConfirmationAfterWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kRedeemRewardConfirmationFeature);

  // Act & Assert
  EXPECT_EQ(base::Seconds(15), kFetchPaymentTokenAfter.Get());
}

}  // namespace brave_ads
