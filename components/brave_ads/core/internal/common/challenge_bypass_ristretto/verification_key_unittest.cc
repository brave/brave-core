/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_key.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_key_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_signature.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_signature_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::cbr {

namespace {
constexpr char kMessage[] = "The quick brown fox jumps over the lazy dog";
}  // namespace

class BraveAdsVerificationKeyTest : public UnitTestBase {};

TEST_F(BraveAdsVerificationKeyTest, Sign) {
  // Arrange
  VerificationKey verification_key = test::GetVerificationKey();

  // Act & Assert
  EXPECT_EQ(test::GetVerificationSignature(), verification_key.Sign(kMessage));
}

TEST_F(BraveAdsVerificationKeyTest, Verify) {
  // Arrange
  VerificationKey verification_key = test::GetVerificationKey();

  // Act & Assert
  EXPECT_TRUE(
      verification_key.Verify(test::GetVerificationSignature(), kMessage));
}

TEST_F(BraveAdsVerificationKeyTest,
       FailToVerifyWithInvalidVerificationSignature) {
  // Arrange
  VerificationKey verification_key = test::GetVerificationKey();

  // Act & Assert
  EXPECT_FALSE(verification_key.Verify(test::GetInvalidVerificationSignature(),
                                       kMessage));
}

}  // namespace brave_ads::cbr
