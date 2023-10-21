/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/redeem_reward_confirmation_util.h"

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRedeemRewardConfirmationUtilTest : public UnitTestBase {};

TEST_F(BraveAdsRedeemRewardConfirmationUtilTest, AddPaymentToken) {
  // Arrange
  const PaymentTokenInfo payment_token = test::BuildPaymentToken();

  // Act
  const auto result = MaybeAddPaymentToken(payment_token);

  // Assert
  EXPECT_TRUE(result.has_value());
}

TEST_F(BraveAdsRedeemRewardConfirmationUtilTest,
       DoNotAddDuplicatePaymentToken) {
  // Arrange
  const PaymentTokenInfo payment_token = test::BuildPaymentToken();

  {
    const auto result = MaybeAddPaymentToken(payment_token);
    ASSERT_TRUE(result.has_value());
  }

  // Act
  const auto result = MaybeAddPaymentToken(payment_token);

  // Assert
  EXPECT_FALSE(result.has_value());
}

TEST_F(BraveAdsRedeemRewardConfirmationUtilTest, LogPaymentTokenStatus) {
  // Act & Assert
  EXPECT_CALL(ads_client_mock_, Log);

  LogPaymentTokenStatus();
}

}  // namespace brave_ads
