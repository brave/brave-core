/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token_util.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace privacy {
namespace cbr {

TEST(BatAdsSignedTokenUtilTest, TokensToRawTokens) {
  // Arrange
  const std::vector<SignedToken> tokens = GetSignedTokens();

  // Act
  const std::vector<challenge_bypass_ristretto::SignedToken> raw_tokens =
      ToRawSignedTokens(tokens);

  // Assert
  std::vector<challenge_bypass_ristretto::SignedToken> expected_raw_tokens;
  for (const auto& token : tokens) {
    expected_raw_tokens.push_back(token.get());
  }

  EXPECT_EQ(expected_raw_tokens, raw_tokens);
}

TEST(BatAdsSignedTokenUtilTest, EmptyTokensToRawTokens) {
  // Arrange
  std::vector<SignedToken> tokens;

  // Act
  const std::vector<challenge_bypass_ristretto::SignedToken> raw_tokens =
      ToRawSignedTokens(tokens);

  // Assert
  EXPECT_TRUE(raw_tokens.empty());
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
