/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPublicKeyTest : public test::TestBase {};

TEST_F(BraveAdsPublicKeyTest, FailToInitialize) {
  // Arrange
  const cbr::PublicKey public_key;

  // Act & Assert
  EXPECT_FALSE(public_key.has_value());
}

TEST_F(BraveAdsPublicKeyTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const cbr::PublicKey public_key("");

  // Act & Assert
  EXPECT_FALSE(public_key.has_value());
}

TEST_F(BraveAdsPublicKeyTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const cbr::PublicKey public_key(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(public_key.has_value());
}

TEST_F(BraveAdsPublicKeyTest, DecodeBase64) {
  // Act
  const cbr::PublicKey public_key =
      cbr::PublicKey::DecodeBase64(cbr::test::kPublicKeyBase64);

  // Assert
  EXPECT_TRUE(public_key.has_value());
}

TEST_F(BraveAdsPublicKeyTest, FailToDecodeEmptyBase64) {
  // Act
  const cbr::PublicKey public_key = cbr::PublicKey::DecodeBase64("");

  // Assert
  EXPECT_FALSE(public_key.has_value());
}

TEST_F(BraveAdsPublicKeyTest, FailToDecodeInvalidBase64) {
  // Act
  const cbr::PublicKey public_key =
      cbr::PublicKey::DecodeBase64(cbr::test::kInvalidBase64);

  // Assert
  EXPECT_FALSE(public_key.has_value());
}

TEST_F(BraveAdsPublicKeyTest, EncodeBase64) {
  // Arrange
  const cbr::PublicKey public_key(cbr::test::kPublicKeyBase64);

  // Act & Assert
  EXPECT_EQ(cbr::test::kPublicKeyBase64, public_key.EncodeBase64());
}

TEST_F(BraveAdsPublicKeyTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const cbr::PublicKey public_key;

  // Act & Assert
  EXPECT_FALSE(public_key.EncodeBase64());
}

TEST_F(BraveAdsPublicKeyTest, IsEqual) {
  // Arrange
  const cbr::PublicKey public_key(cbr::test::kPublicKeyBase64);

  // Act & Assert
  EXPECT_EQ(public_key, public_key);
}

TEST_F(BraveAdsPublicKeyTest, IsEqualWhenUninitialized) {
  // Arrange
  const cbr::PublicKey public_key;

  // Act & Assert
  EXPECT_EQ(public_key, public_key);
}

TEST_F(BraveAdsPublicKeyTest, IsEmptyBase64Equal) {
  // Arrange
  const cbr::PublicKey public_key("");

  // Act & Assert
  EXPECT_EQ(public_key, public_key);
}

TEST_F(BraveAdsPublicKeyTest, IsInvalidBase64Equal) {
  // Arrange
  const cbr::PublicKey public_key(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_EQ(public_key, public_key);
}

TEST_F(BraveAdsPublicKeyTest, IsNotEqual) {
  // Arrange
  const cbr::PublicKey public_key(cbr::test::kPublicKeyBase64);

  // Act & Assert
  const cbr::PublicKey another_public_key(cbr::test::kInvalidBase64);
  EXPECT_NE(another_public_key, public_key);
}

TEST_F(BraveAdsPublicKeyTest, OutputStream) {
  // Arrange
  const cbr::PublicKey public_key(cbr::test::kPublicKeyBase64);

  // Act
  std::stringstream ss;
  ss << public_key;

  // Assert
  EXPECT_EQ(cbr::test::kPublicKeyBase64, ss.str());
}

TEST_F(BraveAdsPublicKeyTest, OutputStreamWhenUninitialized) {
  // Arrange
  const cbr::PublicKey public_key;

  // Act
  std::stringstream ss;
  ss << public_key;

  // Assert
  EXPECT_THAT(ss.str(), ::testing::IsEmpty());
}

}  // namespace brave_ads
