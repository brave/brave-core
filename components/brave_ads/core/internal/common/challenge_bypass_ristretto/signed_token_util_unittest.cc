/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token_util.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::cbr {

TEST(BraveAdsSignedTokenUtilTest, TokensToRawTokens) {
  // Arrange
  const std::vector<SignedToken> tokens = test::GetSignedTokens();

  // Act & Assert
  std::vector<challenge_bypass_ristretto::SignedToken> expected_raw_tokens;
  expected_raw_tokens.reserve(tokens.size());
  for (const auto& token : tokens) {
    expected_raw_tokens.push_back(token.get());
  }
  EXPECT_EQ(expected_raw_tokens, ToRawSignedTokens(tokens));
}

TEST(BraveAdsSignedTokenUtilTest, EmptyTokensToRawTokens) {
  // Arrange
  const std::vector<SignedToken> tokens;

  // Act
  const std::vector<challenge_bypass_ristretto::SignedToken> raw_tokens =
      ToRawSignedTokens(tokens);

  // Assert
  EXPECT_TRUE(raw_tokens.empty());
}

}  // namespace brave_ads::cbr
