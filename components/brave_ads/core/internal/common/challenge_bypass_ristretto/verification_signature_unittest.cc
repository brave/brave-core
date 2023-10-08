/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_signature.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::cbr {

class BraveAdsVerificationSignatureTest : public UnitTestBase {};

TEST_F(BraveAdsVerificationSignatureTest, FailToInitialize) {
  // Arrange
  const VerificationSignature verification_signature;

  // Act & Assert
  EXPECT_FALSE(verification_signature.has_value());
}

TEST_F(BraveAdsVerificationSignatureTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const VerificationSignature verification_signature("");

  // Act & Assert
  EXPECT_FALSE(verification_signature.has_value());
}

TEST_F(BraveAdsVerificationSignatureTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const VerificationSignature verification_signature(kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(verification_signature.has_value());
}

TEST_F(BraveAdsVerificationSignatureTest, DecodeBase64) {
  // Act
  const VerificationSignature verification_signature =
      VerificationSignature::DecodeBase64(kVerificationSignatureBase64);

  // Assert
  EXPECT_TRUE(verification_signature.has_value());
}

TEST_F(BraveAdsVerificationSignatureTest, FailToDecodeEmptyBase64) {
  // Act
  const VerificationSignature verification_signature =
      VerificationSignature::DecodeBase64("");

  // Assert
  EXPECT_FALSE(verification_signature.has_value());
}

TEST_F(BraveAdsVerificationSignatureTest, FailToDecodeInvalidBase64) {
  // Act
  const VerificationSignature verification_signature =
      VerificationSignature::DecodeBase64(kInvalidBase64);

  // Assert
  EXPECT_FALSE(verification_signature.has_value());
}

TEST_F(BraveAdsVerificationSignatureTest, EncodeBase64) {
  // Arrange
  const VerificationSignature verification_signature(
      kVerificationSignatureBase64);

  // Act & Assert
  EXPECT_EQ(kVerificationSignatureBase64,
            verification_signature.EncodeBase64());
}

TEST_F(BraveAdsVerificationSignatureTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const VerificationSignature verification_signature;

  // Act & Assert
  EXPECT_FALSE(verification_signature.EncodeBase64());
}

TEST_F(BraveAdsVerificationSignatureTest, IsEqual) {
  // Arrange
  const VerificationSignature verification_signature(
      kVerificationSignatureBase64);

  // Act & Assert
  EXPECT_EQ(verification_signature, verification_signature);
}

TEST_F(BraveAdsVerificationSignatureTest, IsEqualWhenUninitialized) {
  // Arrange
  const VerificationSignature verification_signature;

  // Act & Assert
  EXPECT_EQ(verification_signature, verification_signature);
}

TEST_F(BraveAdsVerificationSignatureTest, IsEmptyBase64Equal) {
  // Arrange
  const VerificationSignature verification_signature("");

  // Act & Assert
  EXPECT_EQ(verification_signature, verification_signature);
}

TEST_F(BraveAdsVerificationSignatureTest, IsInvalidBase64Equal) {
  // Arrange
  const VerificationSignature verification_signature(kInvalidBase64);

  // Act & Assert
  EXPECT_EQ(verification_signature, verification_signature);
}

TEST_F(BraveAdsVerificationSignatureTest, IsNotEqual) {
  // Arrange
  const VerificationSignature verification_signature(
      kVerificationSignatureBase64);

  // Act & Assert
  const VerificationSignature different_verification_signature(kInvalidBase64);
  EXPECT_NE(different_verification_signature, verification_signature);
}

TEST_F(BraveAdsVerificationSignatureTest, OutputStream) {
  // Arrange
  const VerificationSignature verification_signature(
      kVerificationSignatureBase64);

  // Act
  std::stringstream ss;
  ss << verification_signature;

  // Assert
  EXPECT_EQ(kVerificationSignatureBase64, ss.str());
}

TEST_F(BraveAdsVerificationSignatureTest, OutputStreamWhenUninitialized) {
  // Arrange
  const VerificationSignature verification_signature;

  // Act
  std::stringstream ss;
  ss << verification_signature;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace brave_ads::cbr
