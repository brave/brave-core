/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_key.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_key_unittest_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_signature.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_signature_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::privacy::cbr {

namespace {
constexpr char kMessage[] = "The quick brown fox jumps over the lazy dog";
}  // namespace

TEST(BatAdsVerificationKeyTest, Sign) {
  // Arrange
  VerificationKey verification_key = GetVerificationKey();

  // Act
  const absl::optional<VerificationSignature> verification_signature =
      verification_key.Sign(kMessage);
  ASSERT_TRUE(verification_signature);

  // Assert
  EXPECT_EQ(GetVerificationSignature(), *verification_signature);
}

TEST(BatAdsVerificationKeyTest, Verify) {
  // Arrange
  VerificationKey verification_key = GetVerificationKey();

  // Act
  const bool is_valid =
      verification_key.Verify(GetVerificationSignature(), kMessage);

  // Assert
  EXPECT_TRUE(is_valid);
}

TEST(BatAdsVerificationKeyTest, FailToVerifyWithInvalidVerificationSignature) {
  // Arrange
  VerificationKey verification_key = GetVerificationKey();

  // Act
  const bool is_valid =
      verification_key.Verify(GetInvalidVerificationSignature(), kMessage);

  // Assert
  EXPECT_FALSE(is_valid);
}

}  // namespace ads::privacy::cbr
