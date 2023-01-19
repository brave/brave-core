/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token_preimage.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token_preimage_unittest_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_key.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::privacy::cbr {

TEST(BatAdsUnblindedTokenTest, FailToInitialize) {
  // Arrange
  const UnblindedToken unblinded_token;

  // Act
  const bool has_value = unblinded_token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsUnblindedTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const UnblindedToken unblinded_token("");

  // Act
  const bool has_value = unblinded_token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsUnblindedTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const UnblindedToken unblinded_token(kInvalidBase64);

  // Act
  const bool has_value = unblinded_token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsUnblindedTokenTest, DecodeBase64) {
  // Arrange

  // Act
  const UnblindedToken unblinded_token =
      UnblindedToken::DecodeBase64(kUnblindedTokenBase64);

  // Assert
  const bool has_value = unblinded_token.has_value();
  EXPECT_TRUE(has_value);
}

TEST(BatAdsUnblindedTokenTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const UnblindedToken unblinded_token = UnblindedToken::DecodeBase64({});

  // Assert
  const bool has_value = unblinded_token.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsUnblindedTokenTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const UnblindedToken unblinded_token =
      UnblindedToken::DecodeBase64(kInvalidBase64);

  // Assert
  const bool has_value = unblinded_token.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsUnblindedTokenTest, EncodeBase64) {
  // Arrange
  const UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act
  const absl::optional<std::string> encoded_base64 =
      unblinded_token.EncodeBase64();
  ASSERT_TRUE(encoded_base64);

  // Assert
  EXPECT_EQ(kUnblindedTokenBase64, *encoded_base64);
}

TEST(BatAdsUnblindedTokenTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const UnblindedToken unblinded_token;

  // Act
  const absl::optional<std::string> encoded_base64 =
      unblinded_token.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64);
}

TEST(BatAdsUnblindedTokenTest, IsEqual) {
  // Arrange
  const UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act

  // Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST(BatAdsUnblindedTokenTest, IsEqualWhenUninitialized) {
  // Arrange
  const UnblindedToken unblinded_token;

  // Act

  // Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST(BatAdsUnblindedTokenTest, IsEmptyBase64Equal) {
  // Arrange
  const UnblindedToken unblinded_token("");

  // Act

  // Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST(BatAdsUnblindedTokenTest, IsInvalidBase64Equal) {
  // Arrange
  const UnblindedToken unblinded_token(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST(BatAdsUnblindedTokenTest, IsNotEqual) {
  // Arrange
  const UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act

  // Assert
  const UnblindedToken different_blinded_token(kInvalidBase64);
  EXPECT_NE(different_blinded_token, unblinded_token);
}

TEST(BatAdsUnblindedTokenTest, OutputStream) {
  // Arrange
  const UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act
  std::stringstream ss;
  ss << unblinded_token;

  // Assert
  EXPECT_EQ(kUnblindedTokenBase64, ss.str());
}

TEST(BatAdsUnblindedTokenTest, OutputStreamWhenUninitialized) {
  // Arrange
  const UnblindedToken unblinded_token;

  // Act
  std::stringstream ss;
  ss << unblinded_token;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

TEST(BatAdsUnUnblindedTokenTest, DeriveVerificationKey) {
  // Arrange
  const UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act
  const absl::optional<VerificationKey> verification_key =
      unblinded_token.DeriveVerificationKey();

  // Assert
  EXPECT_TRUE(verification_key);
}

TEST(BatAdsUnUnblindedTokenTest, FailToDeriveVerificationKeyWhenUninitialized) {
  // Arrange
  const UnblindedToken unblinded_token;

  // Act
  const absl::optional<VerificationKey> verification_key =
      unblinded_token.DeriveVerificationKey();

  // Assert
  EXPECT_FALSE(verification_key);
}

TEST(BatAdsUnUnblindedTokenTest,
     FailToDeriveVerificationKeyWithInvalidUnblindedToken) {
  // Arrange
  const UnblindedToken unblinded_token(kInvalidBase64);

  // Act
  const absl::optional<VerificationKey> verification_key =
      unblinded_token.DeriveVerificationKey();

  // Assert
  EXPECT_FALSE(verification_key);
}

TEST(BatAdsUnblindedTokenTest, GetTokenPreimage) {
  // Arrange
  const UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act
  const absl::optional<TokenPreimage> token_preimage =
      unblinded_token.GetTokenPreimage();
  ASSERT_TRUE(token_preimage);

  // Assert
  EXPECT_EQ(GetTokenPreimage(), *token_preimage);
}

TEST(BatAdsUnblindedTokenTest, FailToGetTokenPreimageWhenUninitialized) {
  // Arrange
  const UnblindedToken unblinded_token;

  // Act
  const absl::optional<TokenPreimage> token_preimage =
      unblinded_token.GetTokenPreimage();

  // Assert
  EXPECT_FALSE(token_preimage);
}

TEST(BatAdsUnblindedTokenTest,
     FailToGetTokenPreimageWithInvalidUnblindedToken) {
  // Arrange
  const UnblindedToken unblinded_token(kInvalidBase64);

  // Act
  const absl::optional<TokenPreimage> token_preimage =
      unblinded_token.GetTokenPreimage();

  // Assert
  EXPECT_FALSE(token_preimage);
}

}  // namespace ads::privacy::cbr
