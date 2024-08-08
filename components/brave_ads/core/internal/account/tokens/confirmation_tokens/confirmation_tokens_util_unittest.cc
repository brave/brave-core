/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationTokenUtilTest : public test::TestBase {};

TEST_F(BraveAdsConfirmationTokenUtilTest, GetConfirmationToken) {
  // Arrange
  const ConfirmationTokenList confirmation_tokens =
      test::RefillConfirmationTokens(/*count=*/1);
  ASSERT_THAT(confirmation_tokens, ::testing::SizeIs(1));

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
  ASSERT_THAT(confirmation_tokens, ::testing::SizeIs(2));

  const ConfirmationTokenInfo& token_1 = confirmation_tokens.at(0);
  const ConfirmationTokenInfo& token_2 = confirmation_tokens.at(1);

  GetConfirmationTokens().Set({token_1});

  // Act
  AddConfirmationTokens({token_2});

  // Assert
  const ConfirmationTokenList expected_confirmation_tokens = {token_1, token_2};
  EXPECT_EQ(expected_confirmation_tokens, GetConfirmationTokens().GetAll());
}

TEST_F(BraveAdsConfirmationTokenUtilTest, RemoveConfirmationToken) {
  // Arrange
  const ConfirmationTokenList confirmation_tokens =
      test::BuildConfirmationTokens(/*count=*/3);
  ASSERT_THAT(confirmation_tokens, ::testing::SizeIs(3));

  const ConfirmationTokenInfo& token_1 = confirmation_tokens.at(0);
  const ConfirmationTokenInfo& token_2 = confirmation_tokens.at(1);
  const ConfirmationTokenInfo& token_3 = confirmation_tokens.at(2);

  GetConfirmationTokens().Set(confirmation_tokens);

  // Act
  RemoveConfirmationToken(token_2);

  // Assert
  const ConfirmationTokenList expected_confirmation_tokens = {token_1, token_3};
  EXPECT_EQ(expected_confirmation_tokens, GetConfirmationTokens().GetAll());
}

TEST_F(BraveAdsConfirmationTokenUtilTest, ConfirmationTokenCount) {
  // Arrange
  test::RefillConfirmationTokens(/*count=*/3);

  // Act & Assert
  EXPECT_EQ(3U, ConfirmationTokenCount());
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
