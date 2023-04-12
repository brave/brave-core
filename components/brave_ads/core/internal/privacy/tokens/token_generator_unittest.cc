/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator.h"

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::privacy {

TEST(BatAdsTokenGeneratorTest, Generate) {
  // Arrange
  const TokenGenerator token_generator;

  // Act
  const std::vector<cbr::Token> tokens = token_generator.Generate(5);

  // Assert
  const size_t count = tokens.size();
  EXPECT_EQ(5U, count);
}

TEST(BatAdsTokenGeneratorTest, GenerateZero) {
  // Arrange
  const TokenGenerator token_generator;

  // Act
  const std::vector<cbr::Token> tokens = token_generator.Generate(0);

  // Assert
  EXPECT_TRUE(tokens.empty());
}

}  // namespace brave_ads::privacy
