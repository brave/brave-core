/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/test/payment_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPaymentTokenInfoTest : public test::TestBase {};

TEST_F(BraveAdsPaymentTokenInfoTest, IsValid) {
  // Act & Assert
  EXPECT_TRUE(test::BuildPaymentToken().IsValid());
}

TEST_F(BraveAdsPaymentTokenInfoTest, IsNotValid) {
  // Arrange
  const PaymentTokenInfo payment_token;

  // Act & Assert
  EXPECT_FALSE(payment_token.IsValid());
}

}  // namespace brave_ads
