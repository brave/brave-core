/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token.h"

#include <sstream>

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::privacy::cbr {

TEST(BatAdsSignedTokenTest, FailToInitialize) {
  // Arrange
  const SignedToken signed_token;

  // Act
  const bool has_value = signed_token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsSignedTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const SignedToken signed_token("");

  // Act
  const bool has_value = signed_token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsSignedTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const SignedToken signed_token(kInvalidBase64);

  // Act
  const bool has_value = signed_token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsSignedTokenTest, DecodeBase64) {
  // Arrange

  // Act
  const SignedToken signed_token =
      SignedToken::DecodeBase64(kSignedTokenBase64);

  // Assert
  const bool has_value = signed_token.has_value();
  EXPECT_TRUE(has_value);
}

TEST(BatAdsSignedTokenTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const SignedToken signed_token = SignedToken::DecodeBase64({});

  // Assert
  const bool has_value = signed_token.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsSignedTokenTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const SignedToken signed_token = SignedToken::DecodeBase64(kInvalidBase64);

  // Assert
  const bool has_value = signed_token.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsSignedTokenTest, EncodeBase64) {
  // Arrange
  const SignedToken signed_token(kSignedTokenBase64);

  // Act
  const absl::optional<std::string> encoded_base64 =
      signed_token.EncodeBase64();
  ASSERT_TRUE(encoded_base64);

  // Assert
  EXPECT_EQ(kSignedTokenBase64, *encoded_base64);
}

TEST(BatAdsSignedTokenTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const SignedToken signed_token;

  // Act
  const absl::optional<std::string> encoded_base64 =
      signed_token.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64);
}

TEST(BatAdsSignedTokenTest, IsEqual) {
  // Arrange
  const SignedToken signed_token(kSignedTokenBase64);

  // Act

  // Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST(BatAdsSignedTokenTest, IsEqualWhenUninitialized) {
  // Arrange
  const SignedToken signed_token;

  // Act

  // Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST(BatAdsSignedTokenTest, IsEmptyBase64Equal) {
  // Arrange
  const SignedToken signed_token("");

  // Act

  // Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST(BatAdsSignedTokenTest, IsInvalidBase64Equal) {
  // Arrange
  const SignedToken signed_token(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST(BatAdsSignedTokenTest, IsNotEqual) {
  // Arrange
  const SignedToken signed_token(kSignedTokenBase64);

  // Act

  // Assert
  const SignedToken different_signed_token(kInvalidBase64);
  EXPECT_NE(different_signed_token, signed_token);
}

TEST(BatAdsSignedTokenTest, OutputStream) {
  // Arrange
  const SignedToken signed_token(kSignedTokenBase64);

  // Act
  std::stringstream ss;
  ss << signed_token;

  // Assert
  EXPECT_EQ(kSignedTokenBase64, ss.str());
}

TEST(BatAdsSignedTokenTest, OutputStreamWhenUninitialized) {
  // Arrange
  const SignedToken signed_token;

  // Act
  std::stringstream ss;
  ss << signed_token;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace ads::privacy::cbr
