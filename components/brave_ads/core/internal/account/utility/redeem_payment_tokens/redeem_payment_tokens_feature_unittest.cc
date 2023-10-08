/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsRedeemPaymentTokensFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kRedeemPaymentTokensFeature));
}

TEST(BraveAdsRedeemPaymentTokensFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kRedeemPaymentTokensFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kRedeemPaymentTokensFeature));
}

TEST(BraveAdsRedeemPaymentTokensFeatureTest, RedeemPaymentTokensAfter) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kRedeemPaymentTokensFeature, {{"redeem_after", "3h"}});

  // Act & Assert
  EXPECT_EQ(base::Hours(3), kRedeemPaymentTokensAfter.Get());
}

TEST(BraveAdsRedeemPaymentTokensFeatureTest, DefaultRedeemPaymentTokensAfter) {
  // Act & Assert
  EXPECT_EQ(base::Days(1), kRedeemPaymentTokensAfter.Get());
}

TEST(BraveAdsRedeemPaymentTokensFeatureTest,
     DefaultRedeemPaymentTokensAfterWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kRedeemPaymentTokensFeature);

  // Act & Assert
  EXPECT_EQ(base::Days(1), kRedeemPaymentTokensAfter.Get());
}

}  // namespace brave_ads
