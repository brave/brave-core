/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"

#include <optional>
#include <string>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/test/challenge_bypass_ristretto_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/test/token_preimage_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_preimage.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_signature.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsUnblindedTokenTest : public test::TestBase {};

TEST_F(BraveAdsUnblindedTokenTest, FailToInitialize) {
  // Arrange
  const cbr::UnblindedToken unblinded_token;

  // Act & Assert
  EXPECT_FALSE(unblinded_token.has_value());
}

TEST_F(BraveAdsUnblindedTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const cbr::UnblindedToken unblinded_token("");

  // Act & Assert
  EXPECT_FALSE(unblinded_token.has_value());
}

TEST_F(BraveAdsUnblindedTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const cbr::UnblindedToken unblinded_token(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(unblinded_token.has_value());
}

TEST_F(BraveAdsUnblindedTokenTest, DecodeBase64) {
  // Act
  const cbr::UnblindedToken unblinded_token =
      cbr::UnblindedToken::DecodeBase64(cbr::test::kUnblindedTokenBase64);

  // Assert
  EXPECT_TRUE(unblinded_token.has_value());
}

TEST_F(BraveAdsUnblindedTokenTest, FailToDecodeEmptyBase64) {
  // Act
  const cbr::UnblindedToken unblinded_token =
      cbr::UnblindedToken::DecodeBase64("");

  // Assert
  EXPECT_FALSE(unblinded_token.has_value());
}

TEST_F(BraveAdsUnblindedTokenTest, FailToDecodeInvalidBase64) {
  // Act
  const cbr::UnblindedToken unblinded_token =
      cbr::UnblindedToken::DecodeBase64(cbr::test::kInvalidBase64);

  // Assert
  EXPECT_FALSE(unblinded_token.has_value());
}

TEST_F(BraveAdsUnblindedTokenTest, EncodeBase64) {
  // Arrange
  const cbr::UnblindedToken unblinded_token(cbr::test::kUnblindedTokenBase64);

  // Act & Assert
  EXPECT_EQ(cbr::test::kUnblindedTokenBase64, unblinded_token.EncodeBase64());
}

TEST_F(BraveAdsUnblindedTokenTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const cbr::UnblindedToken unblinded_token;

  // Act & Assert
  EXPECT_FALSE(unblinded_token.EncodeBase64());
}

TEST_F(BraveAdsUnblindedTokenTest, IsEqual) {
  // Arrange
  const cbr::UnblindedToken unblinded_token(cbr::test::kUnblindedTokenBase64);

  // Act & Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST_F(BraveAdsUnblindedTokenTest, IsEqualWhenUninitialized) {
  // Arrange
  const cbr::UnblindedToken unblinded_token;

  // Act & Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST_F(BraveAdsUnblindedTokenTest, IsEmptyBase64Equal) {
  // Arrange
  const cbr::UnblindedToken unblinded_token("");

  // Act & Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST_F(BraveAdsUnblindedTokenTest, IsInvalidBase64Equal) {
  // Arrange
  const cbr::UnblindedToken unblinded_token(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST_F(BraveAdsUnblindedTokenTest, IsNotEqual) {
  // Arrange
  const cbr::UnblindedToken unblinded_token(cbr::test::kUnblindedTokenBase64);

  // Act & Assert
  const cbr::UnblindedToken another_blinded_token(cbr::test::kInvalidBase64);
  EXPECT_NE(another_blinded_token, unblinded_token);
}

TEST_F(BraveAdsUnblindedTokenTest, OutputStream) {
  // Arrange
  const cbr::UnblindedToken unblinded_token(cbr::test::kUnblindedTokenBase64);

  // Act
  std::stringstream ss;
  ss << unblinded_token;

  // Assert
  EXPECT_EQ(cbr::test::kUnblindedTokenBase64, ss.str());
}

TEST_F(BraveAdsUnblindedTokenTest, OutputStreamWhenUninitialized) {
  // Arrange
  const cbr::UnblindedToken unblinded_token;

  // Act
  std::stringstream ss;
  ss << unblinded_token;

  // Assert
  EXPECT_THAT(ss.str(), ::testing::IsEmpty());
}

TEST_F(BraveAdsUnblindedTokenTest, DeriveVerificationKey) {
  // Arrange
  const cbr::UnblindedToken unblinded_token(cbr::test::kUnblindedTokenBase64);

  // Act & Assert
  EXPECT_TRUE(unblinded_token.DeriveVerificationKey());
}

TEST_F(BraveAdsUnblindedTokenTest,
       DeriveVerificationKeySelectsDerivationByRfcFlag) {
  // Arrange
  std::optional<cbr::VerificationKey> rfc_verification_key =
      cbr::UnblindedToken(cbr::test::kUnblindedTokenBase64, /*rfc=*/true)
          .DeriveVerificationKey();
  ASSERT_TRUE(rfc_verification_key);

  std::optional<cbr::VerificationKey> non_rfc_verification_key =
      cbr::UnblindedToken(cbr::test::kUnblindedTokenBase64, /*rfc=*/false)
          .DeriveVerificationKey();
  ASSERT_TRUE(non_rfc_verification_key);

  // Act: sign the same message with each derived key.
  const std::string message = "message";
  std::optional<cbr::VerificationSignature> rfc_signature =
      rfc_verification_key->Sign(message);
  ASSERT_TRUE(rfc_signature);

  std::optional<cbr::VerificationSignature> non_rfc_signature =
      non_rfc_verification_key->Sign(message);
  ASSERT_TRUE(non_rfc_signature);

  // Assert: the `rfc` flag selects a different derivation, so the keys, and
  // thus their signatures over the same message, differ.
  EXPECT_NE(*rfc_signature, *non_rfc_signature);
}

TEST_F(BraveAdsUnblindedTokenTest,
       FailToDeriveVerificationKeyWhenUninitialized) {
  // Arrange
  const cbr::UnblindedToken unblinded_token;

  // Act & Assert
  EXPECT_FALSE(unblinded_token.DeriveVerificationKey());
}

TEST_F(BraveAdsUnblindedTokenTest,
       FailToDeriveVerificationKeyWithInvalidUnblindedToken) {
  // Arrange
  const cbr::UnblindedToken unblinded_token(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(unblinded_token.DeriveVerificationKey());
}

TEST_F(BraveAdsUnblindedTokenTest, GetTokenPreimage) {
  // Arrange
  const cbr::UnblindedToken unblinded_token(cbr::test::kUnblindedTokenBase64);

  // Act & Assert
  EXPECT_EQ(cbr::test::GetTokenPreimage(), unblinded_token.GetTokenPreimage());
}

TEST_F(BraveAdsUnblindedTokenTest, FailToGetTokenPreimageWhenUninitialized) {
  // Arrange
  const cbr::UnblindedToken unblinded_token;

  // Act & Assert
  EXPECT_FALSE(unblinded_token.GetTokenPreimage());
}

TEST_F(BraveAdsUnblindedTokenTest,
       FailToGetTokenPreimageWithInvalidUnblindedToken) {
  // Arrange
  const cbr::UnblindedToken unblinded_token(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(unblinded_token.GetTokenPreimage());
}

}  // namespace brave_ads
