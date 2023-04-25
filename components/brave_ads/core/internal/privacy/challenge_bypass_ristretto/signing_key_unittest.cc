/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signing_key.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signed_token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token_preimage.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token_preimage_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy::cbr {

class BraveAdsSigningKeyTest : public UnitTestBase {};

TEST_F(BraveAdsSigningKeyTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const SigningKey signing_key("");

  // Act

  // Assert
  EXPECT_FALSE(signing_key.has_value());
}

TEST_F(BraveAdsSigningKeyTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const SigningKey signing_key(kInvalidBase64);

  // Act

  // Assert
  EXPECT_FALSE(signing_key.has_value());
}

TEST_F(BraveAdsSigningKeyTest, DecodeBase64) {
  // Arrange

  // Act
  const SigningKey signing_key = SigningKey::DecodeBase64(kSigningKeyBase64);

  // Assert
  EXPECT_TRUE(signing_key.has_value());
}

TEST_F(BraveAdsSigningKeyTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const SigningKey signing_key = SigningKey::DecodeBase64("");

  // Assert
  EXPECT_FALSE(signing_key.has_value());
}

TEST_F(BraveAdsSigningKeyTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const SigningKey signing_key = SigningKey::DecodeBase64(kInvalidBase64);

  // Assert
  EXPECT_FALSE(signing_key.has_value());
}

TEST_F(BraveAdsSigningKeyTest, EncodeBase64) {
  // Arrange
  const SigningKey signing_key(kSigningKeyBase64);

  // Act
  const absl::optional<std::string> encoded_base64 = signing_key.EncodeBase64();
  ASSERT_TRUE(encoded_base64);

  // Assert
  EXPECT_EQ(kSigningKeyBase64, *encoded_base64);
}

TEST_F(BraveAdsSigningKeyTest, Sign) {
  // Arrange
  const SigningKey signing_key(kSigningKeyBase64);

  // Act
  const absl::optional<SignedToken> signed_token =
      signing_key.Sign(GetBlindedToken());
  ASSERT_TRUE(signed_token);

  // Assert
  EXPECT_EQ(GetSignedToken(), *signed_token);
}

TEST_F(BraveAdsSigningKeyTest, FailToSignWithInvalidBlindedToken) {
  // Arrange
  const SigningKey signing_key(kSigningKeyBase64);

  // Act
  const absl::optional<SignedToken> signed_token =
      signing_key.Sign(GetInvalidBlindedToken());

  // Assert
  EXPECT_FALSE(signed_token);
}

TEST_F(BraveAdsSigningKeyTest, RederiveUnblindedToken) {
  // Arrange
  SigningKey signing_key(kSigningKeyBase64);

  // Act
  const absl::optional<UnblindedToken> unblinded_token =
      signing_key.RederiveUnblindedToken(GetTokenPreimage());
  ASSERT_TRUE(unblinded_token);

  // Assert
  EXPECT_EQ(GetUnblindedToken(), *unblinded_token);
}

TEST_F(BraveAdsSigningKeyTest,
       FailToRederiveUnblindedTokenWithInvalidTokenPreimage) {
  // Arrange
  SigningKey signing_key(kSigningKeyBase64);

  // Act
  const absl::optional<UnblindedToken> unblinded_token =
      signing_key.RederiveUnblindedToken(GetInvalidTokenPreimage());

  // Assert
  EXPECT_FALSE(unblinded_token);
}

TEST_F(BraveAdsSigningKeyTest, GetPublicKey) {
  // Arrange
  SigningKey signing_key(kSigningKeyBase64);

  // Act
  const absl::optional<PublicKey> public_key = signing_key.GetPublicKey();
  ASSERT_TRUE(public_key);

  // Assert
  EXPECT_EQ(PublicKey(kPublicKeyBase64), *public_key);
}

TEST_F(BraveAdsSigningKeyTest, IsEqual) {
  // Arrange
  const SigningKey signing_key;

  // Act

  // Assert
  EXPECT_EQ(signing_key, signing_key);
}

TEST_F(BraveAdsSigningKeyTest, IsEmptyBase64Equal) {
  // Arrange
  const SigningKey signing_key("");

  // Act

  // Assert
  EXPECT_EQ(signing_key, signing_key);
}

TEST_F(BraveAdsSigningKeyTest, IsInvalidBase64Equal) {
  // Arrange
  const SigningKey signing_key(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(signing_key, signing_key);
}

TEST_F(BraveAdsSigningKeyTest, IsNotEqual) {
  // Arrange
  const SigningKey signing_key;

  // Act

  // Assert
  const SigningKey different_signing_key;
  EXPECT_NE(different_signing_key, signing_key);
}

TEST_F(BraveAdsSigningKeyTest, OutputStream) {
  // Arrange
  const SigningKey signing_key(kSigningKeyBase64);

  // Act
  std::stringstream ss;
  ss << signing_key;

  // Assert
  EXPECT_EQ(kSigningKeyBase64, ss.str());
}

}  // namespace brave_ads::privacy::cbr
