/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signing_key.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_preimage.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_preimage_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSigningKeyTest : public test::TestBase {};

TEST_F(BraveAdsSigningKeyTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const cbr::SigningKey signing_key("");

  // Act & Assert
  EXPECT_FALSE(signing_key.has_value());
}

TEST_F(BraveAdsSigningKeyTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const cbr::SigningKey signing_key(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(signing_key.has_value());
}

TEST_F(BraveAdsSigningKeyTest, DecodeBase64) {
  // Act
  const cbr::SigningKey signing_key =
      cbr::SigningKey::DecodeBase64(cbr::test::kSigningKeyBase64);

  // Assert
  EXPECT_TRUE(signing_key.has_value());
}

TEST_F(BraveAdsSigningKeyTest, FailToDecodeEmptyBase64) {
  // Act
  const cbr::SigningKey signing_key = cbr::SigningKey::DecodeBase64("");

  // Assert
  EXPECT_FALSE(signing_key.has_value());
}

TEST_F(BraveAdsSigningKeyTest, FailToDecodeInvalidBase64) {
  // Act
  const cbr::SigningKey signing_key =
      cbr::SigningKey::DecodeBase64(cbr::test::kInvalidBase64);

  // Assert
  EXPECT_FALSE(signing_key.has_value());
}

TEST_F(BraveAdsSigningKeyTest, EncodeBase64) {
  // Arrange
  const cbr::SigningKey signing_key(cbr::test::kSigningKeyBase64);

  // Act & Assert
  EXPECT_EQ(cbr::test::kSigningKeyBase64, signing_key.EncodeBase64());
}

TEST_F(BraveAdsSigningKeyTest, Sign) {
  // Arrange
  const cbr::SigningKey signing_key(cbr::test::kSigningKeyBase64);

  // Act & Assert
  EXPECT_EQ(cbr::test::GetSignedToken(),
            signing_key.Sign(cbr::test::GetBlindedToken()));
}

TEST_F(BraveAdsSigningKeyTest, FailToSignWithInvalidBlindedToken) {
  // Arrange
  const cbr::SigningKey signing_key(cbr::test::kSigningKeyBase64);

  // Act & Assert
  EXPECT_FALSE(signing_key.Sign(cbr::test::GetInvalidBlindedToken()));
}

TEST_F(BraveAdsSigningKeyTest, RederiveUnblindedToken) {
  // Arrange
  cbr::SigningKey signing_key(cbr::test::kSigningKeyBase64);

  // Act & Assert
  EXPECT_EQ(cbr::test::GetUnblindedToken(),
            signing_key.RederiveUnblindedToken(cbr::test::GetTokenPreimage()));
}

TEST_F(BraveAdsSigningKeyTest,
       FailToRederiveUnblindedTokenWithInvalidTokenPreimage) {
  // Arrange
  cbr::SigningKey signing_key(cbr::test::kSigningKeyBase64);

  // Act & Assert
  EXPECT_FALSE(
      signing_key.RederiveUnblindedToken(cbr::test::GetInvalidTokenPreimage()));
}

TEST_F(BraveAdsSigningKeyTest, GetPublicKey) {
  // Arrange
  cbr::SigningKey signing_key(cbr::test::kSigningKeyBase64);

  // Act & Assert
  EXPECT_EQ(cbr::PublicKey(cbr::test::kPublicKeyBase64),
            signing_key.GetPublicKey());
}

TEST_F(BraveAdsSigningKeyTest, IsEqual) {
  // Arrange
  const cbr::SigningKey signing_key;

  // Act & Assert
  EXPECT_EQ(signing_key, signing_key);
}

TEST_F(BraveAdsSigningKeyTest, IsEmptyBase64Equal) {
  // Arrange
  const cbr::SigningKey signing_key("");

  // Act & Assert
  EXPECT_EQ(signing_key, signing_key);
}

TEST_F(BraveAdsSigningKeyTest, IsInvalidBase64Equal) {
  // Arrange
  const cbr::SigningKey signing_key(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_EQ(signing_key, signing_key);
}

TEST_F(BraveAdsSigningKeyTest, IsNotEqual) {
  // Arrange
  const cbr::SigningKey signing_key;

  // Act & Assert
  const cbr::SigningKey another_signing_key;
  EXPECT_NE(another_signing_key, signing_key);
}

TEST_F(BraveAdsSigningKeyTest, OutputStream) {
  // Arrange
  const cbr::SigningKey signing_key(cbr::test::kSigningKeyBase64);

  // Act
  std::stringstream ss;
  ss << signing_key;

  // Assert
  EXPECT_EQ(cbr::test::kSigningKeyBase64, ss.str());
}

}  // namespace brave_ads
