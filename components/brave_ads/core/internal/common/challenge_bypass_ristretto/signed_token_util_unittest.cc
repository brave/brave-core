/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token_util.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSignedTokenUtilTest, SignedTokensToRawSignedTokens) {
  // Arrange
  const cbr::SignedTokenList signed_tokens = cbr::test::GetSignedTokens();

  // Act
  const std::vector<challenge_bypass_ristretto::SignedToken> raw_signed_tokens =
      ToRawSignedTokens(signed_tokens);

  // Assert
  std::vector<challenge_bypass_ristretto::SignedToken>
      expected_raw_signed_tokens;
  expected_raw_signed_tokens.reserve(signed_tokens.size());
  for (const auto& signed_token : signed_tokens) {
    expected_raw_signed_tokens.push_back(signed_token.get());
  }
  EXPECT_EQ(expected_raw_signed_tokens, raw_signed_tokens);
}

TEST(BraveAdsSignedTokenUtilTest, EmptySignedTokensToRawSignedTokens) {
  // Act
  const std::vector<challenge_bypass_ristretto::SignedToken> raw_signed_tokens =
      cbr::ToRawSignedTokens({});

  // Assert
  EXPECT_THAT(raw_signed_tokens, ::testing::IsEmpty());
}

}  // namespace brave_ads
