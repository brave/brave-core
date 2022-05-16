/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token_util.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token_unittest_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace privacy {
namespace cbr {

TEST(BatAdsBlindedTokenUtilTest, BlindTokens) {
  // Arrange
  const std::vector<Token> tokens = GetTokens();

  // Act
  const std::vector<BlindedToken> blinded_tokens = BlindTokens(tokens);

  // Assert
  EXPECT_EQ(GetBlindedTokens(), blinded_tokens);
}

TEST(BatAdsBlindedTokenUtilTToUnblindedTokensest, BlindEmptyTokens) {
  // Arrange
  std::vector<Token> tokens;

  // Act
  const std::vector<BlindedToken> blinded_tokens = BlindTokens(tokens);

  // Assert
  EXPECT_TRUE(blinded_tokens.empty());
}

TEST(BatAdsBlindedTokenUtilTest, TokensToRawTokens) {
  // Arrange
  const std::vector<BlindedToken> tokens = GetBlindedTokens();

  // Act
  const std::vector<challenge_bypass_ristretto::BlindedToken> raw_tokens =
      ToRawBlindedTokens(tokens);

  // Assert
  std::vector<challenge_bypass_ristretto::BlindedToken> expected_raw_tokens;
  for (const auto& token : tokens) {
    expected_raw_tokens.push_back(token.get());
  }

  EXPECT_EQ(expected_raw_tokens, raw_tokens);
}

TEST(BatAdsBlindedTokenUtilTest, EmptyTokensToRawTokens) {
  // Arrange
  std::vector<BlindedToken> tokens;

  // Act
  const std::vector<challenge_bypass_ristretto::BlindedToken> raw_tokens =
      ToRawBlindedTokens(tokens);

  // Assert
  EXPECT_TRUE(raw_tokens.empty());
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
