/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_signature.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy::cbr {

TEST(BraveAdsVerificationSignatureTest, FailToInitialize) {
  // Arrange
  const VerificationSignature verification_signature;

  // Act

  // Assert
  EXPECT_FALSE(verification_signature.has_value());
}

TEST(BraveAdsVerificationSignatureTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const VerificationSignature verification_signature("");

  // Act

  // Assert
  EXPECT_FALSE(verification_signature.has_value());
}

TEST(BraveAdsVerificationSignatureTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const VerificationSignature verification_signature(kInvalidBase64);

  // Act

  // Assert
  EXPECT_FALSE(verification_signature.has_value());
}

TEST(BraveAdsVerificationSignatureTest, DecodeBase64) {
  // Arrange

  // Act
  const VerificationSignature verification_signature =
      VerificationSignature::DecodeBase64(kVerificationSignatureBase64);

  // Assert
  EXPECT_TRUE(verification_signature.has_value());
}

TEST(BraveAdsVerificationSignatureTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const VerificationSignature verification_signature =
      VerificationSignature::DecodeBase64("");

  // Assert
  EXPECT_FALSE(verification_signature.has_value());
}

TEST(BraveAdsVerificationSignatureTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const VerificationSignature verification_signature =
      VerificationSignature::DecodeBase64(kInvalidBase64);

  // Assert
  EXPECT_FALSE(verification_signature.has_value());
}

TEST(BraveAdsVerificationSignatureTest, EncodeBase64) {
  // Arrange
  const VerificationSignature verification_signature(
      kVerificationSignatureBase64);

  // Act
  const absl::optional<std::string> encoded_base64 =
      verification_signature.EncodeBase64();
  ASSERT_TRUE(encoded_base64);

  // Assert
  EXPECT_EQ(kVerificationSignatureBase64, *encoded_base64);
}

TEST(BraveAdsVerificationSignatureTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const VerificationSignature verification_signature;

  // Act
  const absl::optional<std::string> encoded_base64 =
      verification_signature.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64);
}

TEST(BraveAdsVerificationSignatureTest, IsEqual) {
  // Arrange
  const VerificationSignature verification_signature(
      kVerificationSignatureBase64);

  // Act

  // Assert
  EXPECT_EQ(verification_signature, verification_signature);
}

TEST(BraveAdsVerificationSignatureTest, IsEqualWhenUninitialized) {
  // Arrange
  const VerificationSignature verification_signature;

  // Act

  // Assert
  EXPECT_EQ(verification_signature, verification_signature);
}

TEST(BraveAdsVerificationSignatureTest, IsEmptyBase64Equal) {
  // Arrange
  const VerificationSignature verification_signature("");

  // Act

  // Assert
  EXPECT_EQ(verification_signature, verification_signature);
}

TEST(BraveAdsVerificationSignatureTest, IsInvalidBase64Equal) {
  // Arrange
  const VerificationSignature verification_signature(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(verification_signature, verification_signature);
}

TEST(BraveAdsVerificationSignatureTest, IsNotEqual) {
  // Arrange
  const VerificationSignature verification_signature(
      kVerificationSignatureBase64);

  // Act

  // Assert
  const VerificationSignature different_verification_signature(kInvalidBase64);
  EXPECT_NE(different_verification_signature, verification_signature);
}

TEST(BraveAdsVerificationSignatureTest, OutputStream) {
  // Arrange
  const VerificationSignature verification_signature(
      kVerificationSignatureBase64);

  // Act
  std::stringstream ss;
  ss << verification_signature;

  // Assert
  EXPECT_EQ(kVerificationSignatureBase64, ss.str());
}

TEST(BraveAdsVerificationSignatureTest, OutputStreamWhenUninitialized) {
  // Arrange
  const VerificationSignature verification_signature;

  // Act
  std::stringstream ss;
  ss << verification_signature;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace brave_ads::privacy::cbr
