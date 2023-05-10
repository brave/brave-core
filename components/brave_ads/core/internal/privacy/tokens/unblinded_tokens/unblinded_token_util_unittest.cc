/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy {

class BraveAdsUnblindedTokenUtilTest : public UnitTestBase {};

TEST_F(BraveAdsUnblindedTokenUtilTest, GetUnblindedToken) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = SetUnblindedTokens(/*count*/ 2);
  ASSERT_EQ(2U, unblinded_tokens.size());

  // Act
  const absl::optional<UnblindedTokenInfo> unblinded_token =
      MaybeGetUnblindedToken();
  ASSERT_TRUE(unblinded_token);

  // Assert
  EXPECT_EQ(unblinded_tokens.front(), *unblinded_token);
}

TEST_F(BraveAdsUnblindedTokenUtilTest, DoNotGetUnblindedToken) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(MaybeGetUnblindedToken());
}

TEST_F(BraveAdsUnblindedTokenUtilTest, AddUnblindedTokens) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = BuildUnblindedTokens(/*count*/ 2);
  ASSERT_EQ(2U, unblinded_tokens.size());

  const UnblindedTokenInfo& token_1 = unblinded_tokens.at(0);
  const UnblindedTokenInfo& token_2 = unblinded_tokens.at(1);

  GetUnblindedTokens().SetTokens({token_1});

  // Act
  AddUnblindedTokens({token_2});

  // Assert
  const UnblindedTokenList expected_tokens = {token_1, token_2};
  EXPECT_EQ(expected_tokens, GetUnblindedTokens().GetAllTokens());
}

TEST_F(BraveAdsUnblindedTokenUtilTest, RemoveUnblindedToken) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = BuildUnblindedTokens(/*count*/ 3);
  ASSERT_EQ(3U, unblinded_tokens.size());

  const UnblindedTokenInfo& token_1 = unblinded_tokens.at(0);
  const UnblindedTokenInfo& token_2 = unblinded_tokens.at(1);
  const UnblindedTokenInfo& token_3 = unblinded_tokens.at(2);

  GetUnblindedTokens().SetTokens(unblinded_tokens);

  // Act
  RemoveUnblindedToken(token_2);

  // Assert
  const UnblindedTokenList expected_tokens = {token_1, token_3};
  EXPECT_EQ(expected_tokens, GetUnblindedTokens().GetAllTokens());
}

TEST_F(BraveAdsUnblindedTokenUtilTest, UnblindedTokenCount) {
  // Arrange
  SetUnblindedTokens(/*count*/ 3);

  // Act

  // Assert
  EXPECT_EQ(3U, UnblindedTokenCount());
}

TEST_F(BraveAdsUnblindedTokenUtilTest, IsValid) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsValid(BuildUnblindedToken()));
}

TEST_F(BraveAdsUnblindedTokenUtilTest, IsNotValid) {
  // Arrange
  const UnblindedTokenInfo unblinded_token;

  // Act

  // Assert
}

}  // namespace brave_ads::privacy
