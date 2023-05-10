/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy {

class BraveAdsUnblindedPaymentTokensTest : public UnitTestBase {};

TEST_F(BraveAdsUnblindedPaymentTokensTest, GetToken) {
  // Arrange
  // Arrange
  const UnblindedPaymentTokenList tokens =
      BuildUnblindedPaymentTokens(/*count*/ 2);
  ASSERT_EQ(2U, tokens.size());

  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens(tokens);

  // Act

  // Assert
  const UnblindedPaymentTokenInfo& expected_token = tokens.at(0);
  EXPECT_EQ(expected_token, unblinded_payment_tokens.GetToken());
}

TEST_F(BraveAdsUnblindedPaymentTokensTest, GetAllTokens) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens(BuildUnblindedPaymentTokens(/*count*/ 2));

  // Act

  // Assert
  EXPECT_EQ(BuildUnblindedPaymentTokens(/*count*/ 2),
            unblinded_payment_tokens.GetAllTokens());
}

TEST_F(BraveAdsUnblindedPaymentTokensTest, SetTokens) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;

  // Act
  unblinded_payment_tokens.SetTokens(BuildUnblindedPaymentTokens(/*count*/ 2));

  // Assert
  EXPECT_EQ(BuildUnblindedPaymentTokens(/*count*/ 2),
            unblinded_payment_tokens.GetAllTokens());
}

TEST_F(BraveAdsUnblindedPaymentTokensTest, SetEmptyTokens) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;

  // Act
  unblinded_payment_tokens.SetTokens({});

  // Assert
  EXPECT_TRUE(unblinded_payment_tokens.IsEmpty());
}

TEST_F(BraveAdsUnblindedPaymentTokensTest, AddTokens) {
  // Arrange
  const UnblindedPaymentTokenList tokens =
      BuildUnblindedPaymentTokens(/*count*/ 2);
  ASSERT_EQ(2U, tokens.size());

  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens({tokens.at(0)});

  // Act
  unblinded_payment_tokens.AddTokens({tokens.at(1)});

  // Assert
  EXPECT_EQ(2U, unblinded_payment_tokens.Count());
}

TEST_F(BraveAdsUnblindedPaymentTokensTest, AddEmptyTokens) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;

  // Act
  unblinded_payment_tokens.AddTokens({});

  // Assert
  EXPECT_TRUE(unblinded_payment_tokens.IsEmpty());
}

TEST_F(BraveAdsUnblindedPaymentTokensTest, DoNotAddDuplicateTokens) {
  // Arrange
  const UnblindedPaymentTokenInfo unblinded_payment_token =
      BuildUnblindedPaymentToken();

  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.AddTokens({unblinded_payment_token});

  // Act
  unblinded_payment_tokens.AddTokens({unblinded_payment_token});

  // Assert
  EXPECT_EQ(1U, unblinded_payment_tokens.Count());
}

TEST_F(BraveAdsUnblindedPaymentTokensTest, RemoveToken) {
  // Arrange
  const UnblindedPaymentTokenList tokens =
      BuildUnblindedPaymentTokens(/*count*/ 2);
  ASSERT_EQ(2U, tokens.size());

  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens(tokens);

  const UnblindedPaymentTokenInfo& token_1 = tokens.at(0);
  const UnblindedPaymentTokenInfo& token_2 = tokens.at(1);

  // Act
  unblinded_payment_tokens.RemoveToken(token_2);

  // Assert
  const UnblindedPaymentTokenList expected_tokens = {token_1};
  EXPECT_EQ(expected_tokens, unblinded_payment_tokens.GetAllTokens());
}

TEST_F(BraveAdsUnblindedPaymentTokensTest, RemoveTokens) {
  // Arrange
  const UnblindedPaymentTokenList tokens =
      BuildUnblindedPaymentTokens(/*count*/ 3);
  ASSERT_EQ(3U, tokens.size());

  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens(tokens);

  const UnblindedPaymentTokenInfo& token_1 = tokens.at(0);
  const UnblindedPaymentTokenInfo& token_2 = tokens.at(1);
  const UnblindedPaymentTokenInfo& token_3 = tokens.at(2);

  // Act
  unblinded_payment_tokens.RemoveTokens({token_1, token_3});

  // Assert
  const UnblindedPaymentTokenList expected_tokens = {token_2};
  EXPECT_EQ(expected_tokens, unblinded_payment_tokens.GetAllTokens());
}

TEST_F(BraveAdsUnblindedPaymentTokensTest, RemoveAllTokens) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens(BuildUnblindedPaymentTokens(/*count*/ 2));

  // Act
  unblinded_payment_tokens.RemoveAllTokens();

  // Assert
  EXPECT_TRUE(unblinded_payment_tokens.IsEmpty());
}

TEST_F(BraveAdsUnblindedPaymentTokensTest, TokenDoesExist) {
  // Arrange
  const UnblindedPaymentTokenInfo unblinded_payment_token =
      BuildUnblindedPaymentToken();

  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens({unblinded_payment_token});

  // Act

  // Assert
  EXPECT_TRUE(unblinded_payment_tokens.TokenExists(unblinded_payment_token));
}

TEST_F(BraveAdsUnblindedPaymentTokensTest, TokenDoesNotExist) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;

  // Act

  // Assert
  EXPECT_FALSE(
      unblinded_payment_tokens.TokenExists(BuildUnblindedPaymentToken()));
}

TEST_F(BraveAdsUnblindedPaymentTokensTest, Count) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens(BuildUnblindedPaymentTokens(/*count*/ 3));

  // Act

  // Assert
  EXPECT_EQ(3U, unblinded_payment_tokens.Count());
}

TEST_F(BraveAdsUnblindedPaymentTokensTest, IsEmpty) {
  // Arrange
  const UnblindedPaymentTokens unblinded_payment_tokens;

  // Act

  // Assert
  EXPECT_TRUE(unblinded_payment_tokens.IsEmpty());
}

TEST_F(BraveAdsUnblindedPaymentTokensTest, IsNotEmpty) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens(BuildUnblindedPaymentTokens(/*count*/ 1));

  // Act

  // Assert
  EXPECT_FALSE(unblinded_payment_tokens.IsEmpty());
}

}  // namespace brave_ads::privacy
