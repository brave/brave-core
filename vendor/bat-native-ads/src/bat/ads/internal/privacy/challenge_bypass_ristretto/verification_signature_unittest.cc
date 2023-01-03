/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_signature.h"

#include <sstream>

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::privacy::cbr {

TEST(BatAdsVerificationSignatureTest, FailToInitialize) {
  // Arrange
  const VerificationSignature verification_signature;

  // Act
  const bool has_value = verification_signature.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsVerificationSignatureTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const VerificationSignature verification_signature("");

  // Act
  const bool has_value = verification_signature.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsVerificationSignatureTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const VerificationSignature verification_signature(kInvalidBase64);

  // Act
  const bool has_value = verification_signature.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsVerificationSignatureTest, DecodeBase64) {
  // Arrange

  // Act
  const VerificationSignature verification_signature =
      VerificationSignature::DecodeBase64(kVerificationSignatureBase64);

  // Assert
  const bool has_value = verification_signature.has_value();
  EXPECT_TRUE(has_value);
}

TEST(BatAdsVerificationSignatureTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const VerificationSignature verification_signature =
      VerificationSignature::DecodeBase64({});

  // Assert
  const bool has_value = verification_signature.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsVerificationSignatureTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const VerificationSignature verification_signature =
      VerificationSignature::DecodeBase64(kInvalidBase64);

  // Assert
  const bool has_value = verification_signature.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsVerificationSignatureTest, EncodeBase64) {
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

TEST(BatAdsVerificationSignatureTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const VerificationSignature verification_signature;

  // Act
  const absl::optional<std::string> encoded_base64 =
      verification_signature.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64);
}

TEST(BatAdsVerificationSignatureTest, IsEqual) {
  // Arrange
  const VerificationSignature verification_signature(
      kVerificationSignatureBase64);

  // Act

  // Assert
  EXPECT_EQ(verification_signature, verification_signature);
}

TEST(BatAdsVerificationSignatureTest, IsEqualWhenUninitialized) {
  // Arrange
  const VerificationSignature verification_signature;

  // Act

  // Assert
  EXPECT_EQ(verification_signature, verification_signature);
}

TEST(BatAdsVerificationSignatureTest, IsEmptyBase64Equal) {
  // Arrange
  const VerificationSignature verification_signature("");

  // Act

  // Assert
  EXPECT_EQ(verification_signature, verification_signature);
}

TEST(BatAdsVerificationSignatureTest, IsInvalidBase64Equal) {
  // Arrange
  const VerificationSignature verification_signature(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(verification_signature, verification_signature);
}

TEST(BatAdsVerificationSignatureTest, IsNotEqual) {
  // Arrange
  const VerificationSignature verification_signature(
      kVerificationSignatureBase64);

  // Act

  // Assert
  const VerificationSignature different_verification_signature(kInvalidBase64);
  EXPECT_NE(different_verification_signature, verification_signature);
}

TEST(BatAdsVerificationSignatureTest, OutputStream) {
  // Arrange
  const VerificationSignature verification_signature(
      kVerificationSignatureBase64);

  // Act
  std::stringstream ss;
  ss << verification_signature;

  // Assert
  EXPECT_EQ(kVerificationSignatureBase64, ss.str());
}

TEST(BatAdsVerificationSignatureTest, OutputStreamWhenUninitialized) {
  // Arrange
  const VerificationSignature verification_signature;

  // Act
  std::stringstream ss;
  ss << verification_signature;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace ads::privacy::cbr
