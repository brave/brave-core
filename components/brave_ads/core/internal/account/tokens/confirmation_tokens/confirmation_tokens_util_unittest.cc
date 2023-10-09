/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationTokenUtilTest : public UnitTestBase {};

TEST_F(BraveAdsConfirmationTokenUtilTest, GetConfirmationToken) {
  // Arrange
  const ConfirmationTokenList confirmation_tokens =
      test::SetConfirmationTokens(/*count=*/2);
  ASSERT_EQ(2U, confirmation_tokens.size());

  // Act & Assert
  EXPECT_EQ(confirmation_tokens.front(), MaybeGetConfirmationToken());
}

TEST_F(BraveAdsConfirmationTokenUtilTest, DoNotGetConfirmationToken) {
  // Act & Assert
  EXPECT_FALSE(MaybeGetConfirmationToken());
}

TEST_F(BraveAdsConfirmationTokenUtilTest, AddConfirmationTokens) {
  // Arrange
  const ConfirmationTokenList confirmation_tokens =
      test::BuildConfirmationTokens(/*count=*/2);
  ASSERT_EQ(2U, confirmation_tokens.size());

  const ConfirmationTokenInfo& token_1 = confirmation_tokens.at(0);
  const ConfirmationTokenInfo& token_2 = confirmation_tokens.at(1);

  test::GetConfirmationTokens().SetTokens({token_1});

  // Act
  AddConfirmationTokens({token_2});

  // Assert
  const ConfirmationTokenList expected_tokens = {token_1, token_2};
  EXPECT_EQ(expected_tokens, test::GetConfirmationTokens().GetAllTokens());
}

TEST_F(BraveAdsConfirmationTokenUtilTest, RemoveConfirmationToken) {
  // Arrange
  const ConfirmationTokenList confirmation_tokens =
      test::BuildConfirmationTokens(/*count=*/3);
  ASSERT_EQ(3U, confirmation_tokens.size());

  const ConfirmationTokenInfo& token_1 = confirmation_tokens.at(0);
  const ConfirmationTokenInfo& token_2 = confirmation_tokens.at(1);
  const ConfirmationTokenInfo& token_3 = confirmation_tokens.at(2);

  test::GetConfirmationTokens().SetTokens(confirmation_tokens);

  // Act
  RemoveConfirmationToken(token_2);

  // Assert
  const ConfirmationTokenList expected_tokens = {token_1, token_3};
  EXPECT_EQ(expected_tokens, test::GetConfirmationTokens().GetAllTokens());
}

TEST_F(BraveAdsConfirmationTokenUtilTest, ConfirmationTokenCount) {
  // Arrange
  test::SetConfirmationTokens(/*count=*/3);

  // Act & Assert
  EXPECT_EQ(3, ConfirmationTokenCount());
}

TEST_F(BraveAdsConfirmationTokenUtilTest, IsValid) {
  // Act & Assert
  EXPECT_TRUE(IsValid(test::BuildConfirmationToken()));
}

TEST_F(BraveAdsConfirmationTokenUtilTest, IsNotValid) {
  // Arrange
  const ConfirmationTokenInfo confirmation_token;

  // Act & Assert
  EXPECT_FALSE(IsValid(confirmation_token));
}

}  // namespace brave_ads
