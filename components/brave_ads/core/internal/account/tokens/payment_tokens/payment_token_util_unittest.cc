/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_util.h"

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPaymentTokenUtilTest : public UnitTestBase {};

TEST_F(BraveAdsPaymentTokenUtilTest, GetPaymentToken) {
  // Arrange
  const PaymentTokenList payment_tokens = test::SetPaymentTokens(/*count=*/2);

  // Act & Assert
  EXPECT_EQ(payment_tokens.front(), MaybeGetPaymentToken());
}

TEST_F(BraveAdsPaymentTokenUtilTest, DoNotGetPaymentToken) {
  // Act & Assert
  EXPECT_FALSE(MaybeGetPaymentToken());
}

TEST_F(BraveAdsPaymentTokenUtilTest, AddPaymentTokens) {
  // Arrange
  const PaymentTokenList payment_tokens = test::BuildPaymentTokens(/*count=*/2);
  ASSERT_EQ(2U, payment_tokens.size());

  const PaymentTokenInfo& token_1 = payment_tokens.at(0);
  const PaymentTokenInfo& token_2 = payment_tokens.at(1);

  test::GetPaymentTokens().SetTokens({token_1});

  // Act
  AddPaymentTokens({token_2});

  // Assert
  const PaymentTokenList expected_tokens = {token_1, token_2};
  EXPECT_EQ(expected_tokens, GetAllPaymentTokens());
}

TEST_F(BraveAdsPaymentTokenUtilTest, RemovePaymentToken) {
  // Arrange
  const PaymentTokenList payment_tokens = test::BuildPaymentTokens(/*count=*/3);
  ASSERT_EQ(3U, payment_tokens.size());

  const PaymentTokenInfo& token_1 = payment_tokens.at(0);
  const PaymentTokenInfo& token_2 = payment_tokens.at(1);
  const PaymentTokenInfo& token_3 = payment_tokens.at(2);

  test::GetPaymentTokens().SetTokens(payment_tokens);

  // Act
  RemovePaymentToken(token_2);

  // Assert
  const PaymentTokenList expected_tokens = {token_1, token_3};
  EXPECT_EQ(expected_tokens, GetAllPaymentTokens());
}

TEST_F(BraveAdsPaymentTokenUtilTest, PaymentTokenCount) {
  // Arrange
  test::SetPaymentTokens(/*count=*/3);

  // Act & Assert
  EXPECT_EQ(3U, PaymentTokenCount());
}

}  // namespace brave_ads
