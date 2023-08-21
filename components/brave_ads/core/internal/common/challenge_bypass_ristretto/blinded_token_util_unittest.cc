/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token_util.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::cbr {

TEST(BraveAdsBlindedTokenUtilTest, BlindTokens) {
  // Arrange
  const std::vector<Token> tokens = GetTokensForTesting();

  // Act
  const std::vector<BlindedToken> blinded_tokens = BlindTokens(tokens);

  // Assert
  EXPECT_EQ(GetBlindedTokensForTesting(), blinded_tokens);
}

TEST(BraveAdsBlindedTokenUtilTToUnblindedTokensest, BlindEmptyTokens) {
  // Arrange

  // Act
  const std::vector<BlindedToken> blinded_tokens = BlindTokens({});

  // Assert
  EXPECT_TRUE(blinded_tokens.empty());
}

TEST(BraveAdsBlindedTokenUtilTest, TokensToRawTokens) {
  // Arrange
  const std::vector<BlindedToken> tokens = GetBlindedTokensForTesting();

  // Act
  const std::vector<challenge_bypass_ristretto::BlindedToken> raw_tokens =
      ToRawBlindedTokens(tokens);

  // Assert
  std::vector<challenge_bypass_ristretto::BlindedToken> expected_raw_tokens;
  expected_raw_tokens.reserve(tokens.size());
  for (const auto& token : tokens) {
    expected_raw_tokens.push_back(token.get());
  }

  EXPECT_EQ(expected_raw_tokens, raw_tokens);
}

TEST(BraveAdsBlindedTokenUtilTest, EmptyTokensToRawTokens) {
  // Arrange

  // Act
  const std::vector<challenge_bypass_ristretto::BlindedToken> raw_tokens =
      ToRawBlindedTokens({});

  // Assert
  EXPECT_TRUE(raw_tokens.empty());
}

}  // namespace brave_ads::cbr
