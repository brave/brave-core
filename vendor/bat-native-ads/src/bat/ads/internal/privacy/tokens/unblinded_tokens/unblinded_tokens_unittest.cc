/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::privacy {

class BatAdsUnblindedTokensTest : public UnitTestBase {};

TEST_F(BatAdsUnblindedTokensTest, GetToken) {
  // Arrange
  // Arrange
  const UnblindedTokenList tokens = GetUnblindedTokens(/*count*/ 2);
  ASSERT_EQ(2U, tokens.size());

  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens(tokens);

  // Act

  // Assert
  const UnblindedTokenInfo& expected_token = tokens.at(0);
  EXPECT_EQ(expected_token, unblinded_tokens.GetToken());
}

TEST_F(BatAdsUnblindedTokensTest, GetAllTokens) {
  // Arrange
  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens(GetUnblindedTokens(/*count*/ 2));

  // Act

  // Assert
  EXPECT_EQ(GetUnblindedTokens(/*count*/ 2), unblinded_tokens.GetAllTokens());
}

TEST_F(BatAdsUnblindedTokensTest, SetTokens) {
  // Arrange
  UnblindedTokens unblinded_tokens;

  // Act
  unblinded_tokens.SetTokens(GetUnblindedTokens(/*count*/ 2));

  // Assert
  EXPECT_EQ(GetUnblindedTokens(/*count*/ 2), unblinded_tokens.GetAllTokens());
}

TEST_F(BatAdsUnblindedTokensTest, SetEmptyTokens) {
  // Arrange
  UnblindedTokens unblinded_tokens;

  // Act
  unblinded_tokens.SetTokens({});

  // Assert
  EXPECT_TRUE(unblinded_tokens.IsEmpty());
}

TEST_F(BatAdsUnblindedTokensTest, AddTokens) {
  // Arrange
  const UnblindedTokenList tokens = GetUnblindedTokens(/*count*/ 2);
  ASSERT_EQ(2U, tokens.size());

  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens({tokens.at(0)});

  // Act
  unblinded_tokens.AddTokens({tokens.at(1)});

  // Assert
  EXPECT_EQ(2, unblinded_tokens.Count());
}

TEST_F(BatAdsUnblindedTokensTest, AddEmptyTokens) {
  // Arrange
  UnblindedTokens unblinded_tokens;

  // Act
  unblinded_tokens.AddTokens({});

  // Assert
  EXPECT_TRUE(unblinded_tokens.IsEmpty());
}

TEST_F(BatAdsUnblindedTokensTest, DoNotAddDuplicateTokens) {
  // Arrange
  const UnblindedTokenInfo unblinded_token = GetUnblindedToken();

  UnblindedTokens unblinded_tokens;
  unblinded_tokens.AddTokens({unblinded_token});

  // Act
  unblinded_tokens.AddTokens({unblinded_token});

  // Assert
  EXPECT_EQ(1, unblinded_tokens.Count());
}

TEST_F(BatAdsUnblindedTokensTest, RemoveToken) {
  // Arrange
  const UnblindedTokenList tokens = GetUnblindedTokens(/*count*/ 2);
  ASSERT_EQ(2U, tokens.size());

  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens(tokens);

  const UnblindedTokenInfo& token_1 = tokens.at(0);
  const UnblindedTokenInfo& token_2 = tokens.at(1);

  // Act
  unblinded_tokens.RemoveToken(token_2);

  // Assert
  const UnblindedTokenList expected_tokens = {token_1};
  EXPECT_EQ(expected_tokens, unblinded_tokens.GetAllTokens());
}

TEST_F(BatAdsUnblindedTokensTest, RemoveTokens) {
  // Arrange
  const UnblindedTokenList tokens = GetUnblindedTokens(/*count*/ 3);
  ASSERT_EQ(3U, tokens.size());

  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens(tokens);

  const UnblindedTokenInfo& token_1 = tokens.at(0);
  const UnblindedTokenInfo& token_2 = tokens.at(1);
  const UnblindedTokenInfo& token_3 = tokens.at(2);

  // Act
  unblinded_tokens.RemoveTokens({token_1, token_3});

  // Assert
  const UnblindedTokenList expected_tokens = {token_2};
  EXPECT_EQ(expected_tokens, unblinded_tokens.GetAllTokens());
}

TEST_F(BatAdsUnblindedTokensTest, RemoveAllTokens) {
  // Arrange
  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens(GetUnblindedTokens(/*count*/ 2));

  // Act
  unblinded_tokens.RemoveAllTokens();

  // Assert
  EXPECT_TRUE(unblinded_tokens.IsEmpty());
}

TEST_F(BatAdsUnblindedTokensTest, TokenDoesExist) {
  // Arrange
  const UnblindedTokenInfo unblinded_token = GetUnblindedToken();

  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens({unblinded_token});

  // Act

  // Assert
  EXPECT_TRUE(unblinded_tokens.TokenExists(unblinded_token));
}

TEST_F(BatAdsUnblindedTokensTest, TokenDoesNotExist) {
  // Arrange
  UnblindedTokens unblinded_tokens;

  // Act

  // Assert
  EXPECT_FALSE(unblinded_tokens.TokenExists(GetUnblindedToken()));
}

TEST_F(BatAdsUnblindedTokensTest, Count) {
  // Arrange
  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens(GetUnblindedTokens(/*count*/ 3));

  // Act

  // Assert
  EXPECT_EQ(3, unblinded_tokens.Count());
}

TEST_F(BatAdsUnblindedTokensTest, IsEmpty) {
  // Arrange
  const UnblindedTokens unblinded_tokens;

  // Act

  // Assert
  EXPECT_TRUE(unblinded_tokens.IsEmpty());
}

TEST_F(BatAdsUnblindedTokensTest, IsNotEmpty) {
  // Arrange
  const UnblindedTokenInfo unblinded_token = GetUnblindedToken();

  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens({unblinded_token});

  // Act

  // Assert
  EXPECT_FALSE(unblinded_tokens.IsEmpty());
}

}  // namespace ads::privacy
