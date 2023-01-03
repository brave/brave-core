/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::privacy {

class BatAdsUnblindedTokenUtilTest : public UnitTestBase {};

TEST_F(BatAdsUnblindedTokenUtilTest, GetUnblindedToken) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(/*count*/ 2);
  ASSERT_EQ(2U, unblinded_tokens.size());

  GetUnblindedTokens()->SetTokens(unblinded_tokens);

  // Act
  const absl::optional<UnblindedTokenInfo> unblinded_token =
      MaybeGetUnblindedToken();
  ASSERT_TRUE(unblinded_token);

  // Assert
  EXPECT_EQ(unblinded_tokens.front(), *unblinded_token);
}

TEST_F(BatAdsUnblindedTokenUtilTest, DoNotGetUnblindedToken) {
  // Arrange

  // Act
  const absl::optional<UnblindedTokenInfo> unblinded_token =
      MaybeGetUnblindedToken();

  // Assert
  EXPECT_FALSE(unblinded_token);
}

TEST_F(BatAdsUnblindedTokenUtilTest, AddUnblindedTokens) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(/*count*/ 2);
  ASSERT_EQ(2U, unblinded_tokens.size());

  const UnblindedTokenInfo& token_1 = unblinded_tokens.at(0);
  const UnblindedTokenInfo& token_2 = unblinded_tokens.at(1);

  GetUnblindedTokens()->SetTokens({token_1});

  // Act
  AddUnblindedTokens({token_2});

  // Assert
  const UnblindedTokenList expected_tokens = {token_1, token_2};
  EXPECT_EQ(expected_tokens, GetUnblindedTokens()->GetAllTokens());
}

TEST_F(BatAdsUnblindedTokenUtilTest, RemoveUnblindedToken) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(/*count*/ 3);
  ASSERT_EQ(3U, unblinded_tokens.size());

  const UnblindedTokenInfo& token_1 = unblinded_tokens.at(0);
  const UnblindedTokenInfo& token_2 = unblinded_tokens.at(1);
  const UnblindedTokenInfo& token_3 = unblinded_tokens.at(2);

  GetUnblindedTokens()->SetTokens(unblinded_tokens);

  // Act
  RemoveUnblindedToken(token_2);

  // Assert
  const UnblindedTokenList expected_tokens = {token_1, token_3};
  EXPECT_EQ(expected_tokens, GetUnblindedTokens()->GetAllTokens());
}

TEST_F(BatAdsUnblindedTokenUtilTest, UnblindedTokenCount) {
  // Arrange
  SetUnblindedTokens(/*count*/ 3);

  // Act

  // Assert
  EXPECT_EQ(3, UnblindedTokenCount());
}

TEST_F(BatAdsUnblindedTokenUtilTest, IsValid) {
  // Arrange
  const UnblindedTokenInfo unblinded_token = GetUnblindedToken();

  // Act

  // Assert
  EXPECT_TRUE(IsValid(unblinded_token));
}

TEST_F(BatAdsUnblindedTokenUtilTest, IsNotValid) {
  // Arrange
  const UnblindedTokenInfo unblinded_token;

  // Act

  // Assert
  EXPECT_FALSE(IsValid(unblinded_token));
}

}  // namespace ads::privacy
