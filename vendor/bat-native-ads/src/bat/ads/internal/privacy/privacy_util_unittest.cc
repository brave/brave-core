/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/privacy_util.h"

#include "bat/ads/internal/privacy/tokens/token_generator.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace privacy {

TEST(BatAdsSecurityUtilsTest, BlindTokens) {
  // Arrange
  TokenGenerator token_generator;
  const std::vector<Token> tokens = token_generator.Generate(7);

  // Act
  const std::vector<BlindedToken> blinded_tokens = BlindTokens(tokens);

  // Assert
  EXPECT_EQ(tokens.size(), blinded_tokens.size());
}

}  // namespace privacy
}  // namespace ads
