/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::privacy {

class BatAdsUnblindedPaymentTokensTest : public UnitTestBase {};

TEST_F(BatAdsUnblindedPaymentTokensTest, GetToken) {
  // Arrange
  // Arrange
  const UnblindedPaymentTokenList tokens =
      GetUnblindedPaymentTokens(/*count*/ 2);
  ASSERT_EQ(2U, tokens.size());

  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens(tokens);

  // Act

  // Assert
  const UnblindedPaymentTokenInfo& expected_token = tokens.at(0);
  EXPECT_EQ(expected_token, unblinded_payment_tokens.GetToken());
}

TEST_F(BatAdsUnblindedPaymentTokensTest, GetAllTokens) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens(GetUnblindedPaymentTokens(/*count*/ 2));

  // Act

  // Assert
  EXPECT_EQ(GetUnblindedPaymentTokens(/*count*/ 2),
            unblinded_payment_tokens.GetAllTokens());
}

TEST_F(BatAdsUnblindedPaymentTokensTest, SetTokens) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;

  // Act
  unblinded_payment_tokens.SetTokens(GetUnblindedPaymentTokens(/*count*/ 2));

  // Assert
  EXPECT_EQ(GetUnblindedPaymentTokens(/*count*/ 2),
            unblinded_payment_tokens.GetAllTokens());
}

TEST_F(BatAdsUnblindedPaymentTokensTest, SetEmptyTokens) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;

  // Act
  unblinded_payment_tokens.SetTokens({});

  // Assert
  EXPECT_TRUE(unblinded_payment_tokens.IsEmpty());
}

TEST_F(BatAdsUnblindedPaymentTokensTest, AddTokens) {
  // Arrange
  const UnblindedPaymentTokenList tokens =
      GetUnblindedPaymentTokens(/*count*/ 2);
  ASSERT_EQ(2U, tokens.size());

  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens({tokens.at(0)});

  // Act
  unblinded_payment_tokens.AddTokens({tokens.at(1)});

  // Assert
  EXPECT_EQ(2, unblinded_payment_tokens.Count());
}

TEST_F(BatAdsUnblindedPaymentTokensTest, AddEmptyTokens) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;

  // Act
  unblinded_payment_tokens.AddTokens({});

  // Assert
  EXPECT_TRUE(unblinded_payment_tokens.IsEmpty());
}

TEST_F(BatAdsUnblindedPaymentTokensTest, DoNotAddDuplicateTokens) {
  // Arrange
  const UnblindedPaymentTokenInfo unblinded_payment_token =
      GetUnblindedPaymentToken();

  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.AddTokens({unblinded_payment_token});

  // Act
  unblinded_payment_tokens.AddTokens({unblinded_payment_token});

  // Assert
  EXPECT_EQ(1, unblinded_payment_tokens.Count());
}

TEST_F(BatAdsUnblindedPaymentTokensTest, RemoveToken) {
  // Arrange
  const UnblindedPaymentTokenList tokens =
      GetUnblindedPaymentTokens(/*count*/ 2);
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

TEST_F(BatAdsUnblindedPaymentTokensTest, RemoveTokens) {
  // Arrange
  const UnblindedPaymentTokenList tokens =
      GetUnblindedPaymentTokens(/*count*/ 3);
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

TEST_F(BatAdsUnblindedPaymentTokensTest, RemoveAllTokens) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens(GetUnblindedPaymentTokens(/*count*/ 2));

  // Act
  unblinded_payment_tokens.RemoveAllTokens();

  // Assert
  EXPECT_TRUE(unblinded_payment_tokens.IsEmpty());
}

TEST_F(BatAdsUnblindedPaymentTokensTest, TokenDoesExist) {
  // Arrange
  const UnblindedPaymentTokenInfo unblinded_payment_token =
      GetUnblindedPaymentToken();

  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens({unblinded_payment_token});

  // Act

  // Assert
  EXPECT_TRUE(unblinded_payment_tokens.TokenExists(unblinded_payment_token));
}

TEST_F(BatAdsUnblindedPaymentTokensTest, TokenDoesNotExist) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;

  // Act

  // Assert
  EXPECT_FALSE(
      unblinded_payment_tokens.TokenExists(GetUnblindedPaymentToken()));
}

TEST_F(BatAdsUnblindedPaymentTokensTest, Count) {
  // Arrange
  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens(GetUnblindedPaymentTokens(/*count*/ 3));

  // Act

  // Assert
  EXPECT_EQ(3, unblinded_payment_tokens.Count());
}

TEST_F(BatAdsUnblindedPaymentTokensTest, IsEmpty) {
  // Arrange
  const UnblindedPaymentTokens unblinded_payment_tokens;

  // Act

  // Assert
  EXPECT_TRUE(unblinded_payment_tokens.IsEmpty());
}

TEST_F(BatAdsUnblindedPaymentTokensTest, IsNotEmpty) {
  // Arrange
  const UnblindedPaymentTokenInfo unblinded_payment_token =
      GetUnblindedPaymentToken();

  UnblindedPaymentTokens unblinded_payment_tokens;
  unblinded_payment_tokens.SetTokens({unblinded_payment_token});

  // Act

  // Assert
  EXPECT_FALSE(unblinded_payment_tokens.IsEmpty());
}

}  // namespace ads::privacy
