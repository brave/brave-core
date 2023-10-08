/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::cbr {

class BraveAdsSignedTokenTest : public UnitTestBase {};

TEST_F(BraveAdsSignedTokenTest, FailToInitialize) {
  // Arrange
  const SignedToken signed_token;

  // Act & Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST_F(BraveAdsSignedTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const SignedToken signed_token("");

  // Act & Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST_F(BraveAdsSignedTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const SignedToken signed_token(kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST_F(BraveAdsSignedTokenTest, DecodeBase64) {
  // Act
  const SignedToken signed_token =
      SignedToken::DecodeBase64(kSignedTokenBase64);

  // Assert
  EXPECT_TRUE(signed_token.has_value());
}

TEST_F(BraveAdsSignedTokenTest, FailToDecodeEmptyBase64) {
  // Act
  const SignedToken signed_token = SignedToken::DecodeBase64("");

  // Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST_F(BraveAdsSignedTokenTest, FailToDecodeInvalidBase64) {
  // Act
  const SignedToken signed_token = SignedToken::DecodeBase64(kInvalidBase64);

  // Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST_F(BraveAdsSignedTokenTest, EncodeBase64) {
  // Arrange
  const SignedToken signed_token(kSignedTokenBase64);

  // Act & Assert
  EXPECT_EQ(kSignedTokenBase64, signed_token.EncodeBase64());
}

TEST_F(BraveAdsSignedTokenTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const SignedToken signed_token;

  // Act & Assert
  EXPECT_FALSE(signed_token.EncodeBase64());
}

TEST_F(BraveAdsSignedTokenTest, IsEqual) {
  // Arrange
  const SignedToken signed_token(kSignedTokenBase64);

  // Act & Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST_F(BraveAdsSignedTokenTest, IsEqualWhenUninitialized) {
  // Arrange
  const SignedToken signed_token;

  // Act & Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST_F(BraveAdsSignedTokenTest, IsEmptyBase64Equal) {
  // Arrange
  const SignedToken signed_token("");

  // Act & Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST_F(BraveAdsSignedTokenTest, IsInvalidBase64Equal) {
  // Arrange
  const SignedToken signed_token(kInvalidBase64);

  // Act & Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST_F(BraveAdsSignedTokenTest, IsNotEqual) {
  // Arrange
  const SignedToken signed_token(kSignedTokenBase64);

  // Act & Assert
  const SignedToken different_signed_token(kInvalidBase64);
  EXPECT_NE(different_signed_token, signed_token);
}

TEST_F(BraveAdsSignedTokenTest, OutputStream) {
  // Arrange
  const SignedToken signed_token(kSignedTokenBase64);

  // Act
  std::stringstream ss;
  ss << signed_token;

  // Assert
  EXPECT_EQ(kSignedTokenBase64, ss.str());
}

TEST_F(BraveAdsSignedTokenTest, OutputStreamWhenUninitialized) {
  // Arrange
  const SignedToken signed_token;

  // Act
  std::stringstream ss;
  ss << signed_token;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace brave_ads::cbr
