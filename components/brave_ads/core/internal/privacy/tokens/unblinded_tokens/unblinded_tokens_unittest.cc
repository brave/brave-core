/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy {

class BraveAdsUnblindedTokensTest : public UnitTestBase {};

TEST_F(BraveAdsUnblindedTokensTest, GetToken) {
  // Arrange
  const UnblindedTokenList tokens = BuildUnblindedTokens(/*count*/ 2);
  ASSERT_EQ(2U, tokens.size());

  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens(tokens);

  // Act

  // Assert
  EXPECT_EQ(tokens.at(0), unblinded_tokens.GetToken());
}

TEST_F(BraveAdsUnblindedTokensTest, GetAllTokens) {
  // Arrange
  const UnblindedTokenList tokens = BuildUnblindedTokens(/*count*/ 2);
  ASSERT_EQ(2U, tokens.size());

  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens(tokens);

  // Act

  // Assert
  EXPECT_EQ(tokens, unblinded_tokens.GetAllTokens());
}

TEST_F(BraveAdsUnblindedTokensTest, SetTokens) {
  // Arrange
  const UnblindedTokenList tokens = BuildUnblindedTokens(/*count*/ 2);
  ASSERT_EQ(2U, tokens.size());

  UnblindedTokens unblinded_tokens;

  // Act
  unblinded_tokens.SetTokens(tokens);

  // Assert
  EXPECT_EQ(tokens, unblinded_tokens.GetAllTokens());
}

TEST_F(BraveAdsUnblindedTokensTest, SetEmptyTokens) {
  // Arrange
  UnblindedTokens unblinded_tokens;

  // Act
  unblinded_tokens.SetTokens({});

  // Assert
  EXPECT_TRUE(unblinded_tokens.IsEmpty());
}

TEST_F(BraveAdsUnblindedTokensTest, AddTokens) {
  // Arrange
  const UnblindedTokenList tokens = BuildUnblindedTokens(/*count*/ 2);
  ASSERT_EQ(2U, tokens.size());

  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens({tokens.at(0)});

  // Act
  unblinded_tokens.AddTokens({tokens.at(1)});

  // Assert
  EXPECT_EQ(2U, unblinded_tokens.Count());
}

TEST_F(BraveAdsUnblindedTokensTest, AddEmptyTokens) {
  // Arrange
  UnblindedTokens unblinded_tokens;

  // Act
  unblinded_tokens.AddTokens({});

  // Assert
  EXPECT_TRUE(unblinded_tokens.IsEmpty());
}

TEST_F(BraveAdsUnblindedTokensTest, DoNotAddDuplicateTokens) {
  // Arrange
  const UnblindedTokenInfo unblinded_token = BuildUnblindedToken();

  UnblindedTokens unblinded_tokens;
  unblinded_tokens.AddTokens({unblinded_token});

  // Act
  unblinded_tokens.AddTokens({unblinded_token});

  // Assert
  EXPECT_EQ(1U, unblinded_tokens.Count());
}

TEST_F(BraveAdsUnblindedTokensTest, RemoveToken) {
  // Arrange
  const UnblindedTokenList tokens = BuildUnblindedTokens(/*count*/ 2);
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

TEST_F(BraveAdsUnblindedTokensTest, RemoveTokens) {
  // Arrange
  const UnblindedTokenList tokens = BuildUnblindedTokens(/*count*/ 3);
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

TEST_F(BraveAdsUnblindedTokensTest, RemoveAllTokens) {
  // Arrange
  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens(BuildUnblindedTokens(/*count*/ 2));

  // Act
  unblinded_tokens.RemoveAllTokens();

  // Assert
  EXPECT_TRUE(unblinded_tokens.IsEmpty());
}

TEST_F(BraveAdsUnblindedTokensTest, TokenDoesExist) {
  // Arrange
  const UnblindedTokenInfo unblinded_token = BuildUnblindedToken();

  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens({unblinded_token});

  // Act

  // Assert
  EXPECT_TRUE(unblinded_tokens.TokenExists(unblinded_token));
}

TEST_F(BraveAdsUnblindedTokensTest, TokenDoesNotExist) {
  // Arrange
  UnblindedTokens unblinded_tokens;

  // Act

  // Assert
  EXPECT_FALSE(unblinded_tokens.TokenExists(BuildUnblindedToken()));
}

TEST_F(BraveAdsUnblindedTokensTest, Count) {
  // Arrange
  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens(BuildUnblindedTokens(/*count*/ 3));

  // Act

  // Assert
  EXPECT_EQ(3U, unblinded_tokens.Count());
}

TEST_F(BraveAdsUnblindedTokensTest, IsEmpty) {
  // Arrange
  const UnblindedTokens unblinded_tokens;

  // Act

  // Assert
  EXPECT_TRUE(unblinded_tokens.IsEmpty());
}

TEST_F(BraveAdsUnblindedTokensTest, IsNotEmpty) {
  // Arrange
  const UnblindedTokenInfo unblinded_token = BuildUnblindedToken();

  UnblindedTokens unblinded_tokens;
  unblinded_tokens.SetTokens({unblinded_token});

  // Act

  // Assert
  EXPECT_FALSE(unblinded_tokens.IsEmpty());
}

}  // namespace brave_ads::privacy
