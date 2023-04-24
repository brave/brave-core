/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy::cbr {

TEST(BraveAdsTokenTest, Random) {
  // Arrange
  const Token token;

  // Act

  // Assert
  EXPECT_TRUE(token.has_value());
}

TEST(BraveAdsTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const Token token("");

  // Act

  // Assert
  EXPECT_FALSE(token.has_value());
}

TEST(BraveAdsTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const Token token(kInvalidBase64);

  // Act

  // Assert
  EXPECT_FALSE(token.has_value());
}

TEST(BraveAdsTokenTest, DecodeBase64) {
  // Arrange

  // Act
  const Token token = Token::DecodeBase64(kTokenBase64);

  // Assert
  EXPECT_TRUE(token.has_value());
}

TEST(BraveAdsTokenTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const Token token = Token::DecodeBase64("");

  // Assert
  EXPECT_FALSE(token.has_value());
}

TEST(BraveAdsTokenTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const Token token = Token::DecodeBase64(kInvalidBase64);

  // Assert
  EXPECT_FALSE(token.has_value());
}

TEST(BraveAdsTokenTest, EncodeBase64) {
  // Arrange
  const Token token(kTokenBase64);

  // Act
  const absl::optional<std::string> encoded_base64 = token.EncodeBase64();
  ASSERT_TRUE(encoded_base64);

  // Assert
  EXPECT_EQ(kTokenBase64, *encoded_base64);
}

TEST(BraveAdsTokenTest, EncodeRandomBase64) {
  // Arrange
  const Token token;

  // Act
  const absl::optional<std::string> encoded_base64 = token.EncodeBase64();

  // Assert
  EXPECT_TRUE(encoded_base64);
}

TEST(BraveAdsTokenTest, IsEqual) {
  // Arrange
  const Token token;

  // Act

  // Assert
  EXPECT_EQ(token, token);
}

TEST(BraveAdsTokenTest, IsEmptyBase64Equal) {
  // Arrange
  const Token token("");

  // Act

  // Assert
  EXPECT_EQ(token, token);
}

TEST(BraveAdsTokenTest, IsInvalidBase64Equal) {
  // Arrange
  const Token token(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(token, token);
}

TEST(BraveAdsTokenTest, IsNotEqual) {
  // Arrange
  const Token token;

  // Act

  // Assert
  const Token different_token;
  EXPECT_NE(different_token, token);
}

TEST(BraveAdsTokenTest, OutputStream) {
  // Arrange
  const Token token(kTokenBase64);

  // Act
  std::stringstream ss;
  ss << token;

  // Assert
  EXPECT_EQ(kTokenBase64, ss.str());
}

}  // namespace brave_ads::privacy::cbr
