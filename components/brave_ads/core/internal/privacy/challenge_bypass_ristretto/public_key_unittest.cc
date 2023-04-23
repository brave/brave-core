/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy::cbr {

TEST(BraveAdsPublicKeyTest, FailToInitialize) {
  // Arrange
  const PublicKey public_key;

  // Act

  // Assert
  EXPECT_FALSE(public_key.has_value());
}

TEST(BraveAdsPublicKeyTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const PublicKey public_key("");

  // Act

  // Assert
  EXPECT_FALSE(public_key.has_value());
}

TEST(BraveAdsPublicKeyTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const PublicKey public_key(kInvalidBase64);

  // Act

  // Assert
  EXPECT_FALSE(public_key.has_value());
}

TEST(BraveAdsPublicKeyTest, DecodeBase64) {
  // Arrange

  // Act
  const PublicKey public_key = PublicKey::DecodeBase64(kPublicKeyBase64);

  // Assert
  EXPECT_TRUE(public_key.has_value());
}

TEST(BraveAdsPublicKeyTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const PublicKey public_key = PublicKey::DecodeBase64({});

  // Assert
  EXPECT_FALSE(public_key.has_value());
}

TEST(BraveAdsPublicKeyTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const PublicKey public_key = PublicKey::DecodeBase64(kInvalidBase64);

  // Assert
  EXPECT_FALSE(public_key.has_value());
}

TEST(BraveAdsPublicKeyTest, EncodeBase64) {
  // Arrange
  const PublicKey public_key(kPublicKeyBase64);

  // Act
  const absl::optional<std::string> encoded_base64 = public_key.EncodeBase64();
  ASSERT_TRUE(encoded_base64);

  // Assert
  EXPECT_EQ(kPublicKeyBase64, *encoded_base64);
}

TEST(BraveAdsPublicKeyTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const PublicKey public_key;

  // Act
  const absl::optional<std::string> encoded_base64 = public_key.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64);
}

TEST(BraveAdsPublicKeyTest, IsEqual) {
  // Arrange
  const PublicKey public_key(kPublicKeyBase64);

  // Act

  // Assert
  EXPECT_EQ(public_key, public_key);
}

TEST(BraveAdsPublicKeyTest, IsEqualWhenUninitialized) {
  // Arrange
  const PublicKey public_key;

  // Act

  // Assert
  EXPECT_EQ(public_key, public_key);
}

TEST(BraveAdsPublicKeyTest, IsEmptyBase64Equal) {
  // Arrange
  const PublicKey public_key("");

  // Act

  // Assert
  EXPECT_EQ(public_key, public_key);
}

TEST(BraveAdsPublicKeyTest, IsInvalidBase64Equal) {
  // Arrange
  const PublicKey public_key(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(public_key, public_key);
}

TEST(BraveAdsPublicKeyTest, IsNotEqual) {
  // Arrange
  const PublicKey public_key(kPublicKeyBase64);

  // Act

  // Assert
  const PublicKey different_public_key(kInvalidBase64);
  EXPECT_NE(different_public_key, public_key);
}

TEST(BraveAdsPublicKeyTest, OutputStream) {
  // Arrange
  const PublicKey public_key(kPublicKeyBase64);

  // Act
  std::stringstream ss;
  ss << public_key;

  // Assert
  EXPECT_EQ(kPublicKeyBase64, ss.str());
}

TEST(BraveAdsPublicKeyTest, OutputStreamWhenUninitialized) {
  // Arrange
  const PublicKey public_key;

  // Act
  std::stringstream ss;
  ss << public_key;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace brave_ads::privacy::cbr
