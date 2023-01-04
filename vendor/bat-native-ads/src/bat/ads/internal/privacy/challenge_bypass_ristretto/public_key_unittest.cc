/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key.h"

#include <sstream>

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::privacy::cbr {

TEST(BatAdsPublicKeyTest, FailToInitialize) {
  // Arrange
  const PublicKey public_key;

  // Act
  const bool has_value = public_key.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsPublicKeyTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const PublicKey public_key("");

  // Act
  const bool has_value = public_key.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsPublicKeyTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const PublicKey public_key(kInvalidBase64);

  // Act
  const bool has_value = public_key.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsPublicKeyTest, DecodeBase64) {
  // Arrange

  // Act
  const PublicKey public_key = PublicKey::DecodeBase64(kPublicKeyBase64);

  // Assert
  const bool has_value = public_key.has_value();
  EXPECT_TRUE(has_value);
}

TEST(BatAdsPublicKeyTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const PublicKey public_key = PublicKey::DecodeBase64({});

  // Assert
  const bool has_value = public_key.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsPublicKeyTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const PublicKey public_key = PublicKey::DecodeBase64(kInvalidBase64);

  // Assert
  const bool has_value = public_key.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsPublicKeyTest, EncodeBase64) {
  // Arrange
  const PublicKey public_key(kPublicKeyBase64);

  // Act
  const absl::optional<std::string> encoded_base64 = public_key.EncodeBase64();
  ASSERT_TRUE(encoded_base64);

  // Assert
  EXPECT_EQ(kPublicKeyBase64, *encoded_base64);
}

TEST(BatAdsPublicKeyTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const PublicKey public_key;

  // Act
  const absl::optional<std::string> encoded_base64 = public_key.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64);
}

TEST(BatAdsPublicKeyTest, IsEqual) {
  // Arrange
  const PublicKey public_key(kPublicKeyBase64);

  // Act

  // Assert
  EXPECT_EQ(public_key, public_key);
}

TEST(BatAdsPublicKeyTest, IsEqualWhenUninitialized) {
  // Arrange
  const PublicKey public_key;

  // Act

  // Assert
  EXPECT_EQ(public_key, public_key);
}

TEST(BatAdsPublicKeyTest, IsEmptyBase64Equal) {
  // Arrange
  const PublicKey public_key("");

  // Act

  // Assert
  EXPECT_EQ(public_key, public_key);
}

TEST(BatAdsPublicKeyTest, IsInvalidBase64Equal) {
  // Arrange
  const PublicKey public_key(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(public_key, public_key);
}

TEST(BatAdsPublicKeyTest, IsNotEqual) {
  // Arrange
  const PublicKey public_key(kPublicKeyBase64);

  // Act

  // Assert
  const PublicKey different_public_key(kInvalidBase64);
  EXPECT_NE(different_public_key, public_key);
}

TEST(BatAdsPublicKeyTest, OutputStream) {
  // Arrange
  const PublicKey public_key(kPublicKeyBase64);

  // Act
  std::stringstream ss;
  ss << public_key;

  // Assert
  EXPECT_EQ(kPublicKeyBase64, ss.str());
}

TEST(BatAdsPublicKeyTest, OutputStreamWhenUninitialized) {
  // Arrange
  const PublicKey public_key;

  // Act
  std::stringstream ss;
  ss << public_key;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace ads::privacy::cbr
