/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy {

class BraveAdsUnblindedPaymentTokenUtilTest : public UnitTestBase {};

TEST_F(BraveAdsUnblindedPaymentTokenUtilTest, GetUnblindedPaymentToken) {
  // Arrange
  const UnblindedPaymentTokenList unblinded_payment_tokens =
      privacy::SetUnblindedPaymentTokens(/*count*/ 2);

  // Act
  const absl::optional<UnblindedPaymentTokenInfo> unblinded_payment_token =
      MaybeGetUnblindedPaymentToken();
  ASSERT_TRUE(unblinded_payment_token);

  // Assert
  EXPECT_EQ(unblinded_payment_tokens.front(), *unblinded_payment_token);
}

TEST_F(BraveAdsUnblindedPaymentTokenUtilTest, DoNotGetUnblindedPaymentToken) {
  // Arrange

  // Act
  const absl::optional<UnblindedPaymentTokenInfo> unblinded_payment_token =
      MaybeGetUnblindedPaymentToken();

  // Assert
  EXPECT_FALSE(unblinded_payment_token);
}

TEST_F(BraveAdsUnblindedPaymentTokenUtilTest, AddUnblindedPaymentTokens) {
  // Arrange
  const UnblindedPaymentTokenList unblinded_payment_tokens =
      BuildUnblindedPaymentTokens(/*count*/ 2);
  ASSERT_EQ(2U, unblinded_payment_tokens.size());

  const UnblindedPaymentTokenInfo& token_1 = unblinded_payment_tokens.at(0);
  const UnblindedPaymentTokenInfo& token_2 = unblinded_payment_tokens.at(1);

  GetUnblindedPaymentTokens().SetTokens({token_1});

  // Act
  AddUnblindedPaymentTokens({token_2});

  // Assert
  const UnblindedPaymentTokenList expected_tokens = {token_1, token_2};
  EXPECT_EQ(expected_tokens, GetAllUnblindedPaymentTokens());
}

TEST_F(BraveAdsUnblindedPaymentTokenUtilTest, RemoveUnblindedPaymentToken) {
  // Arrange
  const UnblindedPaymentTokenList unblinded_payment_tokens =
      BuildUnblindedPaymentTokens(/*count*/ 3);
  ASSERT_EQ(3U, unblinded_payment_tokens.size());

  const UnblindedPaymentTokenInfo& token_1 = unblinded_payment_tokens.at(0);
  const UnblindedPaymentTokenInfo& token_2 = unblinded_payment_tokens.at(1);
  const UnblindedPaymentTokenInfo& token_3 = unblinded_payment_tokens.at(2);

  GetUnblindedPaymentTokens().SetTokens(unblinded_payment_tokens);

  // Act
  RemoveUnblindedPaymentToken(token_2);

  // Assert
  const UnblindedPaymentTokenList expected_tokens = {token_1, token_3};
  EXPECT_EQ(expected_tokens, GetAllUnblindedPaymentTokens());
}

TEST_F(BraveAdsUnblindedPaymentTokenUtilTest, UnblindedPaymentTokenCount) {
  // Arrange
  SetUnblindedPaymentTokens(/*count*/ 3);

  // Act

  // Assert
  EXPECT_EQ(3U, UnblindedPaymentTokenCount());
}

}  // namespace brave_ads::privacy
