/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/token_generator.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsTokenGeneratorTest, Generate) {
  // Arrange
  const TokenGenerator token_generator;

  // Act
  const cbr::TokenList tokens = token_generator.Generate(5);

  // Assert
  EXPECT_THAT(tokens, ::testing::SizeIs(5));
}

TEST(BraveAdsTokenGeneratorTest, GenerateZero) {
  // Arrange
  const TokenGenerator token_generator;

  // Act
  const cbr::TokenList tokens = token_generator.Generate(0);

  // Assert
  EXPECT_THAT(tokens, ::testing::IsEmpty());
}

}  // namespace brave_ads
