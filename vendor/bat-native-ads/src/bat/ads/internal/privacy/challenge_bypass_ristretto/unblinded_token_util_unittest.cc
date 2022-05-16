/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token_util.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace privacy {
namespace cbr {

TEST(BatAdsUnblindedTokenUtilTest, RawTokensToTokens) {
  // Arrange
  std::vector<challenge_bypass_ristretto::UnblindedToken> raw_tokens;
  const UnblindedToken unblinded_token = GetUnblindedToken();
  raw_tokens.push_back(unblinded_token.get());

  // Act
  const std::vector<UnblindedToken> tokens = ToUnblindedTokens(raw_tokens);

  // Assert
  std::vector<UnblindedToken> expected_tokens;
  for (const auto& raw_token : raw_tokens) {
    expected_tokens.push_back(UnblindedToken(raw_token));
  }

  EXPECT_EQ(expected_tokens, tokens);
}

TEST(BatAdsTokenUtilTest, EmptyRawTokensToTokens) {
  // Arrange
  std::vector<challenge_bypass_ristretto::UnblindedToken> raw_tokens;

  // Act
  const std::vector<UnblindedToken> tokens = ToUnblindedTokens(raw_tokens);

  // Assert
  EXPECT_TRUE(tokens.empty());
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
