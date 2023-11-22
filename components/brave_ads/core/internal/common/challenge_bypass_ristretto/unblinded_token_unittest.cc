/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_preimage.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_preimage_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_key.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::cbr {

class BraveAdsUnblindedTokenTest : public UnitTestBase {};

TEST_F(BraveAdsUnblindedTokenTest, FailToInitialize) {
  // Arrange
  const UnblindedToken unblinded_token;

  // Act & Assert
  EXPECT_FALSE(unblinded_token.has_value());
}

TEST_F(BraveAdsUnblindedTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const UnblindedToken unblinded_token("");

  // Act & Assert
  EXPECT_FALSE(unblinded_token.has_value());
}

TEST_F(BraveAdsUnblindedTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const UnblindedToken unblinded_token(kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(unblinded_token.has_value());
}

TEST_F(BraveAdsUnblindedTokenTest, DecodeBase64) {
  // Act
  const UnblindedToken unblinded_token =
      UnblindedToken::DecodeBase64(kUnblindedTokenBase64);

  // Assert
  EXPECT_TRUE(unblinded_token.has_value());
}

TEST_F(BraveAdsUnblindedTokenTest, FailToDecodeEmptyBase64) {
  // Act
  const UnblindedToken unblinded_token = UnblindedToken::DecodeBase64("");

  // Assert
  EXPECT_FALSE(unblinded_token.has_value());
}

TEST_F(BraveAdsUnblindedTokenTest, FailToDecodeInvalidBase64) {
  // Act
  const UnblindedToken unblinded_token =
      UnblindedToken::DecodeBase64(kInvalidBase64);

  // Assert
  EXPECT_FALSE(unblinded_token.has_value());
}

TEST_F(BraveAdsUnblindedTokenTest, EncodeBase64) {
  // Arrange
  const UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act & Assert
  EXPECT_EQ(kUnblindedTokenBase64, unblinded_token.EncodeBase64());
}

TEST_F(BraveAdsUnblindedTokenTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const UnblindedToken unblinded_token;

  // Act & Assert
  EXPECT_FALSE(unblinded_token.EncodeBase64());
}

TEST_F(BraveAdsUnblindedTokenTest, IsEqual) {
  // Arrange
  const UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act & Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST_F(BraveAdsUnblindedTokenTest, IsEqualWhenUninitialized) {
  // Arrange
  const UnblindedToken unblinded_token;

  // Act & Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST_F(BraveAdsUnblindedTokenTest, IsEmptyBase64Equal) {
  // Arrange
  const UnblindedToken unblinded_token("");

  // Act & Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST_F(BraveAdsUnblindedTokenTest, IsInvalidBase64Equal) {
  // Arrange
  const UnblindedToken unblinded_token(kInvalidBase64);

  // Act & Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST_F(BraveAdsUnblindedTokenTest, IsNotEqual) {
  // Arrange
  const UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act & Assert
  const UnblindedToken different_blinded_token(kInvalidBase64);
  EXPECT_NE(different_blinded_token, unblinded_token);
}

TEST_F(BraveAdsUnblindedTokenTest, OutputStream) {
  // Arrange
  const UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act
  std::stringstream ss;
  ss << unblinded_token;

  // Assert
  EXPECT_EQ(kUnblindedTokenBase64, ss.str());
}

TEST_F(BraveAdsUnblindedTokenTest, OutputStreamWhenUninitialized) {
  // Arrange
  const UnblindedToken unblinded_token;

  // Act
  std::stringstream ss;
  ss << unblinded_token;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

TEST_F(BraveAdsUnblindedTokenTest, DeriveVerificationKey) {
  // Arrange
  const UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act & Assert
  EXPECT_TRUE(unblinded_token.DeriveVerificationKey());
}

TEST_F(BraveAdsUnblindedTokenTest,
       FailToDeriveVerificationKeyWhenUninitialized) {
  // Arrange
  const UnblindedToken unblinded_token;

  // Act & Assert
  EXPECT_FALSE(unblinded_token.DeriveVerificationKey());
}

TEST_F(BraveAdsUnblindedTokenTest,
       FailToDeriveVerificationKeyWithInvalidUnblindedToken) {
  // Arrange
  const UnblindedToken unblinded_token(kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(unblinded_token.DeriveVerificationKey());
}

TEST_F(BraveAdsUnblindedTokenTest, GetTokenPreimage) {
  // Arrange
  const UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act & Assert
  EXPECT_EQ(test::GetTokenPreimage(), unblinded_token.GetTokenPreimage());
}

TEST_F(BraveAdsUnblindedTokenTest, FailToGetTokenPreimageWhenUninitialized) {
  // Arrange
  const UnblindedToken unblinded_token;

  // Act & Assert
  EXPECT_FALSE(unblinded_token.GetTokenPreimage());
}

TEST_F(BraveAdsUnblindedTokenTest,
       FailToGetTokenPreimageWithInvalidUnblindedToken) {
  // Arrange
  const UnblindedToken unblinded_token(kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(unblinded_token.GetTokenPreimage());
}

}  // namespace brave_ads::cbr
