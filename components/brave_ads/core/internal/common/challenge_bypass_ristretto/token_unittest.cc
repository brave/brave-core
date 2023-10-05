/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::cbr {

class BraveAdsTokenTest : public UnitTestBase {};

TEST_F(BraveAdsTokenTest, Random) {
  // Arrange
  const Token token;

  // Act & Assert
  EXPECT_TRUE(token.has_value());
}

TEST_F(BraveAdsTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const Token token("");

  // Act & Assert
  EXPECT_FALSE(token.has_value());
}

TEST_F(BraveAdsTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const Token token(kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(token.has_value());
}

TEST_F(BraveAdsTokenTest, DecodeBase64) {
  // Act
  const Token token = Token::DecodeBase64(kTokenBase64);

  // Assert
  EXPECT_TRUE(token.has_value());
}

TEST_F(BraveAdsTokenTest, FailToDecodeEmptyBase64) {
  // Act
  const Token token = Token::DecodeBase64("");

  // Assert
  EXPECT_FALSE(token.has_value());
}

TEST_F(BraveAdsTokenTest, FailToDecodeInvalidBase64) {
  // Act
  const Token token = Token::DecodeBase64(kInvalidBase64);

  // Assert
  EXPECT_FALSE(token.has_value());
}

TEST_F(BraveAdsTokenTest, EncodeBase64) {
  // Arrange
  const Token token(kTokenBase64);

  // Act & Assert
  EXPECT_EQ(kTokenBase64, token.EncodeBase64());
}

TEST_F(BraveAdsTokenTest, EncodeRandomBase64) {
  // Arrange
  const Token token;

  // Act & Assert
  EXPECT_TRUE(token.EncodeBase64());
}

TEST_F(BraveAdsTokenTest, IsEqual) {
  // Arrange
  const Token token;

  // Act & Assert
  EXPECT_EQ(token, token);
}

TEST_F(BraveAdsTokenTest, IsEmptyBase64Equal) {
  // Arrange
  const Token token("");

  // Act & Assert
  EXPECT_EQ(token, token);
}

TEST_F(BraveAdsTokenTest, IsInvalidBase64Equal) {
  // Arrange
  const Token token(kInvalidBase64);

  // Act & Assert
  EXPECT_EQ(token, token);
}

TEST_F(BraveAdsTokenTest, IsNotEqual) {
  // Arrange
  const Token token;

  // Act & Assert
  const Token different_token;
  EXPECT_NE(different_token, token);
}

TEST_F(BraveAdsTokenTest, OutputStream) {
  // Arrange
  const Token token(kTokenBase64);

  // Act
  std::stringstream ss;
  ss << token;

  // Assert
  EXPECT_EQ(kTokenBase64, ss.str());
}

}  // namespace brave_ads::cbr
