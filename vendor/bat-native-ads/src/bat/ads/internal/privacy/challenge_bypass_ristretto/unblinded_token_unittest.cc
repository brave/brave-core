/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token_preimage.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token_preimage_unittest_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_key.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace privacy {
namespace cbr {

TEST(BatAdsUnblindedTokenTest, FailToInitialize) {
  // Arrange
  UnblindedToken unblinded_token;

  // Act
  const bool has_value = unblinded_token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsUnblindedTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  UnblindedToken unblinded_token("");

  // Act
  const bool has_value = unblinded_token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsUnblindedTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  UnblindedToken unblinded_token(kInvalidBase64);

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
  const UnblindedToken unblinded_token = UnblindedToken::DecodeBase64("");

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
  UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act
  const absl::optional<std::string> encoded_base64_optional =
      unblinded_token.EncodeBase64();
  ASSERT_TRUE(encoded_base64_optional);

  const std::string& encoded_base64 = encoded_base64_optional.value();

  // Assert
  EXPECT_EQ(kUnblindedTokenBase64, encoded_base64);
}

TEST(BatAdsUnblindedTokenTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  UnblindedToken unblinded_token;

  // Act
  const absl::optional<std::string> encoded_base64_optional =
      unblinded_token.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64_optional);
}

TEST(BatAdsUnblindedTokenTest, IsEqual) {
  // Arrange
  UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act

  // Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST(BatAdsUnblindedTokenTest, IsEqualWhenUninitialized) {
  // Arrange
  UnblindedToken unblinded_token;

  // Act

  // Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST(BatAdsUnblindedTokenTest, IsEmptyBase64Equal) {
  // Arrange
  UnblindedToken unblinded_token("");

  // Act

  // Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST(BatAdsUnblindedTokenTest, IsInvalidBase64Equal) {
  // Arrange
  UnblindedToken unblinded_token(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(unblinded_token, unblinded_token);
}

TEST(BatAdsUnblindedTokenTest, IsNotEqual) {
  // Arrange
  UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act

  // Assert
  UnblindedToken different_blinded_token(kInvalidBase64);
  EXPECT_NE(different_blinded_token, unblinded_token);
}

TEST(BatAdsUnblindedTokenTest, OutputStream) {
  // Arrange
  UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act
  std::stringstream ss;
  ss << unblinded_token;

  // Assert
  EXPECT_EQ(kUnblindedTokenBase64, ss.str());
}

TEST(BatAdsUnblindedTokenTest, OutputStreamWhenUninitialized) {
  // Arrange
  UnblindedToken unblinded_token;

  // Act
  std::stringstream ss;
  ss << unblinded_token;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

TEST(BatAdsUnUnblindedTokenTest, DeriveVerificationKey) {
  // Arrange
  UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act
  const absl::optional<VerificationKey> verification_key_optional =
      unblinded_token.DeriveVerificationKey();

  // Assert
  EXPECT_TRUE(verification_key_optional);
}

TEST(BatAdsUnUnblindedTokenTest, FailToDeriveVerificationKeyWhenUninitialized) {
  // Arrange
  UnblindedToken unblinded_token;

  // Act
  const absl::optional<VerificationKey> verification_key_optional =
      unblinded_token.DeriveVerificationKey();

  // Assert
  EXPECT_FALSE(verification_key_optional);
}

TEST(BatAdsUnUnblindedTokenTest,
     FailToDeriveVerificationKeyWithInvalidUnblindedToken) {
  // Arrange
  UnblindedToken unblinded_token(kInvalidBase64);

  // Act
  const absl::optional<VerificationKey> verification_key_optional =
      unblinded_token.DeriveVerificationKey();

  // Assert
  EXPECT_FALSE(verification_key_optional);
}

TEST(BatAdsUnblindedTokenTest, GetTokenPreimage) {
  // Arrange
  UnblindedToken unblinded_token(kUnblindedTokenBase64);

  // Act
  const absl::optional<TokenPreimage> token_preimage_optional =
      unblinded_token.GetTokenPreimage();
  ASSERT_TRUE(token_preimage_optional);
  const TokenPreimage& token_preimage = token_preimage_optional.value();

  // Assert
  EXPECT_EQ(GetTokenPreimage(), token_preimage);
}

TEST(BatAdsUnblindedTokenTest, FailToGetTokenPreimageWhenUninitialized) {
  // Arrange
  UnblindedToken unblinded_token;

  // Act
  const absl::optional<TokenPreimage> token_preimage_optional =
      unblinded_token.GetTokenPreimage();

  // Assert
  EXPECT_FALSE(token_preimage_optional);
}

TEST(BatAdsUnblindedTokenTest,
     FailToGetTokenPreimageWithInvalidUnblindedToken) {
  // Arrange
  UnblindedToken unblinded_token(kInvalidBase64);

  // Act
  const absl::optional<TokenPreimage> token_preimage_optional =
      unblinded_token.GetTokenPreimage();

  // Assert
  EXPECT_FALSE(token_preimage_optional);
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
