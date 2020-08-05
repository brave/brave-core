/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/privacy_util.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsSecurityUtilsTest,
    GenerateTokens) {
  // Arrange

  // Act
  const std::vector<privacy::Token> tokens = privacy::GenerateTokens(5);

  // Assert
  const size_t count = tokens.size();
  EXPECT_EQ(5UL, count);
}

TEST(BatAdsSecurityUtilsTest,
    BlindTokens) {
  // Arrange
  const std::vector<privacy::Token> tokens = privacy::GenerateTokens(7);

  // Act
  const std::vector<privacy::BlindedToken> blinded_tokens =
      privacy::BlindTokens(tokens);

  // Assert
  EXPECT_EQ(tokens.size(), blinded_tokens.size());
}

}  // namespace ads
