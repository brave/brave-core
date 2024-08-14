/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens.h"

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationTokensTest : public test::TestBase {};

TEST_F(BraveAdsConfirmationTokensTest, GetToken) {
  // Arrange
  const ConfirmationTokenList tokens =
      test::BuildConfirmationTokens(/*count=*/2);
  ASSERT_THAT(tokens, ::testing::SizeIs(2));

  ConfirmationTokens confirmation_tokens;
  confirmation_tokens.Set(tokens);

  // Act & Assert
  EXPECT_EQ(tokens.front(), confirmation_tokens.Get());
}

TEST_F(BraveAdsConfirmationTokensTest, GetAllTokens) {
  // Arrange
  const ConfirmationTokenList tokens =
      test::BuildConfirmationTokens(/*count=*/2);
  ASSERT_THAT(tokens, ::testing::SizeIs(2));

  ConfirmationTokens confirmation_tokens;
  confirmation_tokens.Set(tokens);

  // Act & Assert
  EXPECT_EQ(tokens, confirmation_tokens.GetAll());
}

TEST_F(BraveAdsConfirmationTokensTest, SetTokens) {
  // Arrange
  const ConfirmationTokenList tokens =
      test::BuildConfirmationTokens(/*count=*/2);
  ASSERT_THAT(tokens, ::testing::SizeIs(2));

  ConfirmationTokens confirmation_tokens;

  // Act
  confirmation_tokens.Set(tokens);

  // Assert
  EXPECT_EQ(tokens, confirmation_tokens.GetAll());
}

TEST_F(BraveAdsConfirmationTokensTest, SetEmptyTokens) {
  // Arrange
  ConfirmationTokens confirmation_tokens;

  // Act
  confirmation_tokens.Set({});

  // Assert
  EXPECT_TRUE(confirmation_tokens.IsEmpty());
}

TEST_F(BraveAdsConfirmationTokensTest, AddTokens) {
  // Arrange
  const ConfirmationTokenList tokens =
      test::BuildConfirmationTokens(/*count=*/2);
  ASSERT_THAT(tokens, ::testing::SizeIs(2));

  ConfirmationTokens confirmation_tokens;
  confirmation_tokens.Set({tokens.at(0)});

  // Act
  confirmation_tokens.Add({tokens.at(1)});

  // Assert
  EXPECT_EQ(2U, confirmation_tokens.Count());
}

TEST_F(BraveAdsConfirmationTokensTest, AddEmptyTokens) {
  // Arrange
  ConfirmationTokens confirmation_tokens;

  // Act
  confirmation_tokens.Add({});

  // Assert
  EXPECT_TRUE(confirmation_tokens.IsEmpty());
}

TEST_F(BraveAdsConfirmationTokensTest, DoNotAddDuplicateTokens) {
  // Arrange
  const ConfirmationTokenInfo confirmation_token =
      test::BuildConfirmationToken();

  ConfirmationTokens confirmation_tokens;
  confirmation_tokens.Add({confirmation_token});

  // Act
  confirmation_tokens.Add({confirmation_token});

  // Assert
  EXPECT_EQ(1U, confirmation_tokens.Count());
}

TEST_F(BraveAdsConfirmationTokensTest, RemoveToken) {
  // Arrange
  const ConfirmationTokenList tokens =
      test::BuildConfirmationTokens(/*count=*/2);
  ASSERT_THAT(tokens, ::testing::SizeIs(2));

  ConfirmationTokens confirmation_tokens;
  confirmation_tokens.Set(tokens);

  const ConfirmationTokenInfo& token_1 = tokens.at(0);
  const ConfirmationTokenInfo& token_2 = tokens.at(1);

  // Act
  confirmation_tokens.Remove(token_2);

  // Assert
  EXPECT_EQ(ConfirmationTokenList{token_1}, confirmation_tokens.GetAll());
}

TEST_F(BraveAdsConfirmationTokensTest, RemoveTokens) {
  // Arrange
  const ConfirmationTokenList tokens =
      test::BuildConfirmationTokens(/*count=*/3);
  ASSERT_THAT(tokens, ::testing::SizeIs(3));

  ConfirmationTokens confirmation_tokens;
  confirmation_tokens.Set(tokens);

  const ConfirmationTokenInfo& token_1 = tokens.at(0);
  const ConfirmationTokenInfo& token_2 = tokens.at(1);
  const ConfirmationTokenInfo& token_3 = tokens.at(2);

  // Act
  confirmation_tokens.Remove({token_1, token_3});

  // Assert
  EXPECT_EQ(ConfirmationTokenList{token_2}, confirmation_tokens.GetAll());
}

TEST_F(BraveAdsConfirmationTokensTest, RemoveAllTokens) {
  // Arrange
  ConfirmationTokens confirmation_tokens;
  confirmation_tokens.Set(test::BuildConfirmationTokens(/*count=*/2));

  // Act
  confirmation_tokens.RemoveAll();

  // Assert
  EXPECT_TRUE(confirmation_tokens.IsEmpty());
}

TEST_F(BraveAdsConfirmationTokensTest, TokenDoesExist) {
  // Arrange
  const ConfirmationTokenInfo confirmation_token =
      test::BuildConfirmationToken();

  ConfirmationTokens confirmation_tokens;
  confirmation_tokens.Set({confirmation_token});

  // Act & Assert
  EXPECT_TRUE(confirmation_tokens.Exists(confirmation_token));
}

TEST_F(BraveAdsConfirmationTokensTest, TokenDoesNotExist) {
  // Arrange
  ConfirmationTokens confirmation_tokens;

  // Act & Assert
  EXPECT_FALSE(confirmation_tokens.Exists(test::BuildConfirmationToken()));
}

TEST_F(BraveAdsConfirmationTokensTest, Count) {
  // Arrange
  ConfirmationTokens confirmation_tokens;
  confirmation_tokens.Set(test::BuildConfirmationTokens(/*count=*/3));

  // Act & Assert
  EXPECT_EQ(3U, confirmation_tokens.Count());
}

TEST_F(BraveAdsConfirmationTokensTest, IsEmpty) {
  // Arrange
  const ConfirmationTokens confirmation_tokens;

  // Act & Assert
  EXPECT_TRUE(confirmation_tokens.IsEmpty());
}

TEST_F(BraveAdsConfirmationTokensTest, IsNotEmpty) {
  // Arrange
  const ConfirmationTokenInfo confirmation_token =
      test::BuildConfirmationToken();

  ConfirmationTokens confirmation_tokens;
  confirmation_tokens.Set({confirmation_token});

  // Act & Assert
  EXPECT_FALSE(confirmation_tokens.IsEmpty());
}

}  // namespace brave_ads
