/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signed_token.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy::cbr {

TEST(BraveAdsSignedTokenTest, FailToInitialize) {
  // Arrange
  const SignedToken signed_token;

  // Act

  // Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST(BraveAdsSignedTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const SignedToken signed_token("");

  // Act

  // Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST(BraveAdsSignedTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const SignedToken signed_token(kInvalidBase64);

  // Act

  // Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST(BraveAdsSignedTokenTest, DecodeBase64) {
  // Arrange

  // Act
  const SignedToken signed_token =
      SignedToken::DecodeBase64(kSignedTokenBase64);

  // Assert
  EXPECT_TRUE(signed_token.has_value());
}

TEST(BraveAdsSignedTokenTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const SignedToken signed_token = SignedToken::DecodeBase64({});

  // Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST(BraveAdsSignedTokenTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const SignedToken signed_token = SignedToken::DecodeBase64(kInvalidBase64);

  // Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST(BraveAdsSignedTokenTest, EncodeBase64) {
  // Arrange
  const SignedToken signed_token(kSignedTokenBase64);

  // Act
  const absl::optional<std::string> encoded_base64 =
      signed_token.EncodeBase64();
  ASSERT_TRUE(encoded_base64);

  // Assert
  EXPECT_EQ(kSignedTokenBase64, *encoded_base64);
}

TEST(BraveAdsSignedTokenTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const SignedToken signed_token;

  // Act
  const absl::optional<std::string> encoded_base64 =
      signed_token.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64);
}

TEST(BraveAdsSignedTokenTest, IsEqual) {
  // Arrange
  const SignedToken signed_token(kSignedTokenBase64);

  // Act

  // Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST(BraveAdsSignedTokenTest, IsEqualWhenUninitialized) {
  // Arrange
  const SignedToken signed_token;

  // Act

  // Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST(BraveAdsSignedTokenTest, IsEmptyBase64Equal) {
  // Arrange
  const SignedToken signed_token("");

  // Act

  // Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST(BraveAdsSignedTokenTest, IsInvalidBase64Equal) {
  // Arrange
  const SignedToken signed_token(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST(BraveAdsSignedTokenTest, IsNotEqual) {
  // Arrange
  const SignedToken signed_token(kSignedTokenBase64);

  // Act

  // Assert
  const SignedToken different_signed_token(kInvalidBase64);
  EXPECT_NE(different_signed_token, signed_token);
}

TEST(BraveAdsSignedTokenTest, OutputStream) {
  // Arrange
  const SignedToken signed_token(kSignedTokenBase64);

  // Act
  std::stringstream ss;
  ss << signed_token;

  // Assert
  EXPECT_EQ(kSignedTokenBase64, ss.str());
}

TEST(BraveAdsSignedTokenTest, OutputStreamWhenUninitialized) {
  // Arrange
  const SignedToken signed_token;

  // Act
  std::stringstream ss;
  ss << signed_token;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace brave_ads::privacy::cbr
