/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens.h"

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPaymentTokensTest : public UnitTestBase {};

TEST_F(BraveAdsPaymentTokensTest, GetToken) {
  // Arrange
  const PaymentTokenList tokens = test::BuildPaymentTokens(/*count=*/2);
  ASSERT_EQ(2U, tokens.size());

  PaymentTokens payment_tokens;
  payment_tokens.SetTokens(tokens);

  // Act & Assert
  const PaymentTokenInfo& expected_token = tokens.at(0);
  EXPECT_EQ(expected_token, payment_tokens.GetToken());
}

TEST_F(BraveAdsPaymentTokensTest, GetAllTokens) {
  // Arrange
  PaymentTokens payment_tokens;
  payment_tokens.SetTokens(test::BuildPaymentTokens(/*count=*/2));

  // Act & Assert
  EXPECT_EQ(test::BuildPaymentTokens(/*count=*/2),
            payment_tokens.GetAllTokens());
}

TEST_F(BraveAdsPaymentTokensTest, SetTokens) {
  // Arrange
  PaymentTokens payment_tokens;

  // Act
  payment_tokens.SetTokens(test::BuildPaymentTokens(/*count=*/2));

  // Assert
  EXPECT_EQ(test::BuildPaymentTokens(/*count=*/2),
            payment_tokens.GetAllTokens());
}

TEST_F(BraveAdsPaymentTokensTest, SetEmptyTokens) {
  // Arrange
  PaymentTokens payment_tokens;

  // Act
  payment_tokens.SetTokens({});

  // Assert
  EXPECT_TRUE(payment_tokens.IsEmpty());
}

TEST_F(BraveAdsPaymentTokensTest, AddTokens) {
  // Arrange
  const PaymentTokenList tokens = test::BuildPaymentTokens(/*count=*/2);
  ASSERT_EQ(2U, tokens.size());

  PaymentTokens payment_tokens;
  payment_tokens.SetTokens({tokens.at(0)});

  // Act
  payment_tokens.AddTokens({tokens.at(1)});

  // Assert
  EXPECT_EQ(2U, payment_tokens.Count());
}

TEST_F(BraveAdsPaymentTokensTest, AddEmptyTokens) {
  // Arrange
  PaymentTokens payment_tokens;

  // Act
  payment_tokens.AddTokens({});

  // Assert
  EXPECT_TRUE(payment_tokens.IsEmpty());
}

TEST_F(BraveAdsPaymentTokensTest, DoNotAddDuplicateTokens) {
  // Arrange
  const PaymentTokenInfo payment_token = test::BuildPaymentToken();

  PaymentTokens payment_tokens;
  payment_tokens.AddTokens({payment_token});

  // Act
  payment_tokens.AddTokens({payment_token});

  // Assert
  EXPECT_EQ(1U, payment_tokens.Count());
}

TEST_F(BraveAdsPaymentTokensTest, RemoveToken) {
  // Arrange
  const PaymentTokenList tokens = test::BuildPaymentTokens(/*count=*/2);
  ASSERT_EQ(2U, tokens.size());

  PaymentTokens payment_tokens;
  payment_tokens.SetTokens(tokens);

  const PaymentTokenInfo& token_1 = tokens.at(0);
  const PaymentTokenInfo& token_2 = tokens.at(1);

  // Act
  payment_tokens.RemoveToken(token_2);

  // Assert
  EXPECT_EQ(PaymentTokenList{token_1}, payment_tokens.GetAllTokens());
}

TEST_F(BraveAdsPaymentTokensTest, RemoveTokens) {
  // Arrange
  const PaymentTokenList tokens = test::BuildPaymentTokens(/*count=*/3);
  ASSERT_EQ(3U, tokens.size());

  PaymentTokens payment_tokens;
  payment_tokens.SetTokens(tokens);

  const PaymentTokenInfo& token_1 = tokens.at(0);
  const PaymentTokenInfo& token_2 = tokens.at(1);
  const PaymentTokenInfo& token_3 = tokens.at(2);

  // Act
  payment_tokens.RemoveTokens({token_1, token_3});

  // Assert
  EXPECT_EQ(PaymentTokenList{token_2}, payment_tokens.GetAllTokens());
}

TEST_F(BraveAdsPaymentTokensTest, RemoveAllTokens) {
  // Arrange
  PaymentTokens payment_tokens;
  payment_tokens.SetTokens(test::BuildPaymentTokens(/*count=*/2));

  // Act
  payment_tokens.RemoveAllTokens();

  // Assert
  EXPECT_TRUE(payment_tokens.IsEmpty());
}

TEST_F(BraveAdsPaymentTokensTest, TokenDoesExist) {
  // Arrange
  const PaymentTokenInfo payment_token = test::BuildPaymentToken();

  PaymentTokens payment_tokens;
  payment_tokens.SetTokens({payment_token});

  // Act & Assert
  EXPECT_TRUE(payment_tokens.TokenExists(payment_token));
}

TEST_F(BraveAdsPaymentTokensTest, TokenDoesNotExist) {
  // Arrange
  PaymentTokens payment_tokens;

  // Act & Assert
  EXPECT_FALSE(payment_tokens.TokenExists(test::BuildPaymentToken()));
}

TEST_F(BraveAdsPaymentTokensTest, Count) {
  // Arrange
  PaymentTokens payment_tokens;
  payment_tokens.SetTokens(test::BuildPaymentTokens(/*count=*/3));

  // Act & Assert
  EXPECT_EQ(3U, payment_tokens.Count());
}

TEST_F(BraveAdsPaymentTokensTest, IsEmpty) {
  // Arrange
  const PaymentTokens payment_tokens;

  // Act & Assert
  EXPECT_TRUE(payment_tokens.IsEmpty());
}

TEST_F(BraveAdsPaymentTokensTest, IsNotEmpty) {
  // Arrange
  PaymentTokens payment_tokens;
  payment_tokens.SetTokens(test::BuildPaymentTokens(/*count=*/1));

  // Act & Assert
  EXPECT_FALSE(payment_tokens.IsEmpty());
}

}  // namespace brave_ads
