/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_key.h"

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_key_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_signature.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_signature_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy::cbr {

namespace {
constexpr char kMessage[] = "The quick brown fox jumps over the lazy dog";
}  // namespace

TEST(BraveAdsVerificationKeyTest, Sign) {
  // Arrange
  VerificationKey verification_key = GetVerificationKey();

  // Act
  const absl::optional<VerificationSignature> verification_signature =
      verification_key.Sign(kMessage);
  ASSERT_TRUE(verification_signature);

  // Assert
  EXPECT_EQ(GetVerificationSignature(), *verification_signature);
}

TEST(BraveAdsVerificationKeyTest, Verify) {
  // Arrange
  VerificationKey verification_key = GetVerificationKey();

  // Act
  // Assert
  EXPECT_TRUE(verification_key.Verify(GetVerificationSignature(), kMessage));
}

TEST(BraveAdsVerificationKeyTest,
     FailToVerifyWithInvalidVerificationSignature) {
  // Arrange
  VerificationKey verification_key = GetVerificationKey();

  // Act

  // Assert
  EXPECT_FALSE(
      verification_key.Verify(GetInvalidVerificationSignature(), kMessage));
}

}  // namespace brave_ads::privacy::cbr
