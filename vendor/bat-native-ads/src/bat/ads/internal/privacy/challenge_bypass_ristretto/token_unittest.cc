/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"

#include <sstream>

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::privacy::cbr {

TEST(BatAdsTokenTest, Random) {
  // Arrange
  const Token token;

  // Act
  const bool has_value = token.has_value();

  // Assert
  EXPECT_TRUE(has_value);
}

TEST(BatAdsTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const Token token("");

  // Act
  const bool has_value = token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const Token token(kInvalidBase64);

  // Act
  const bool has_value = token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsTokenTest, DecodeBase64) {
  // Arrange

  // Act
  const Token token = Token::DecodeBase64(kTokenBase64);

  // Assert
  const bool has_value = token.has_value();
  EXPECT_TRUE(has_value);
}

TEST(BatAdsTokenTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const Token token = Token::DecodeBase64({});

  // Assert
  const bool has_value = token.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsTokenTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const Token token = Token::DecodeBase64(kInvalidBase64);

  // Assert
  const bool has_value = token.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsTokenTest, EncodeBase64) {
  // Arrange
  const Token token(kTokenBase64);

  // Act
  const absl::optional<std::string> encoded_base64 = token.EncodeBase64();
  ASSERT_TRUE(encoded_base64);

  // Assert
  EXPECT_EQ(kTokenBase64, *encoded_base64);
}

TEST(BatAdsTokenTest, EncodeRandomBase64) {
  // Arrange
  const Token token;

  // Act
  const absl::optional<std::string> encoded_base64 = token.EncodeBase64();

  // Assert
  EXPECT_TRUE(encoded_base64);
}

TEST(BatAdsTokenTest, IsEqual) {
  // Arrange
  const Token token;

  // Act

  // Assert
  EXPECT_EQ(token, token);
}

TEST(BatAdsTokenTest, IsEmptyBase64Equal) {
  // Arrange
  const Token token("");

  // Act

  // Assert
  EXPECT_EQ(token, token);
}

TEST(BatAdsTokenTest, IsInvalidBase64Equal) {
  // Arrange
  const Token token(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(token, token);
}

TEST(BatAdsTokenTest, IsNotEqual) {
  // Arrange
  const Token token;

  // Act

  // Assert
  const Token different_token;
  EXPECT_NE(different_token, token);
}

TEST(BatAdsTokenTest, OutputStream) {
  // Arrange
  const Token token(kTokenBase64);

  // Act
  std::stringstream ss;
  ss << token;

  // Assert
  EXPECT_EQ(kTokenBase64, ss.str());
}

}  // namespace ads::privacy::cbr
