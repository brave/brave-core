/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token_preimage.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy::cbr {

TEST(BraveAdsTokenPreimageTest, FailToInitialize) {
  // Arrange
  const TokenPreimage token_preimage;

  // Act

  // Assert
  EXPECT_FALSE(token_preimage.has_value());
}

TEST(BraveAdsTokenPreimageTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const TokenPreimage token_preimage("");

  // Act

  // Assert
  EXPECT_FALSE(token_preimage.has_value());
}

TEST(BraveAdsTokenPreimageTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const TokenPreimage token_preimage(kInvalidBase64);

  // Act

  // Assert
  EXPECT_FALSE(token_preimage.has_value());
}

TEST(BraveAdsTokenPreimageTest, DecodeBase64) {
  // Arrange

  // Act
  const TokenPreimage token_preimage =
      TokenPreimage::DecodeBase64(kTokenPreimageBase64);

  // Assert
  EXPECT_TRUE(token_preimage.has_value());
}

TEST(BraveAdsTokenPreimageTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const TokenPreimage token_preimage = TokenPreimage::DecodeBase64({});

  // Assert
  EXPECT_FALSE(token_preimage.has_value());
}

TEST(BraveAdsTokenPreimageTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const TokenPreimage token_preimage =
      TokenPreimage::DecodeBase64(kInvalidBase64);

  // Assert
  EXPECT_FALSE(token_preimage.has_value());
}

TEST(BraveAdsTokenPreimageTest, EncodeBase64) {
  // Arrange
  const TokenPreimage token_preimage(kTokenPreimageBase64);

  // Act
  const absl::optional<std::string> encoded_base64 =
      token_preimage.EncodeBase64();
  ASSERT_TRUE(encoded_base64);

  // Assert
  EXPECT_EQ(kTokenPreimageBase64, *encoded_base64);
}

TEST(BraveAdsTokenPreimageTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const TokenPreimage token_preimage;

  // Act
  const absl::optional<std::string> encoded_base64 =
      token_preimage.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64);
}

TEST(BraveAdsTokenPreimageTest, IsEqual) {
  // Arrange
  const TokenPreimage token_preimage(kTokenPreimageBase64);

  // Act

  // Assert
  EXPECT_EQ(token_preimage, token_preimage);
}

TEST(BraveAdsTokenPreimageTest, IsEqualWhenUninitialized) {
  // Arrange
  const TokenPreimage token_preimage;

  // Act

  // Assert
  EXPECT_EQ(token_preimage, token_preimage);
}

TEST(BraveAdsTokenPreimageTest, IsEmptyBase64Equal) {
  // Arrange
  const TokenPreimage token_preimage("");

  // Act

  // Assert
  EXPECT_EQ(token_preimage, token_preimage);
}

TEST(BraveAdsTokenPreimageTest, IsInvalidBase64Equal) {
  // Arrange
  const TokenPreimage token_preimage(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(token_preimage, token_preimage);
}

TEST(BraveAdsTokenPreimageTest, IsNotEqual) {
  // Arrange
  const TokenPreimage token_preimage(kTokenPreimageBase64);

  // Act

  // Assert
  const TokenPreimage different_token_preimage(kInvalidBase64);
  EXPECT_NE(different_token_preimage, token_preimage);
}

TEST(BraveAdsTokenPreimageTest, OutputStream) {
  // Arrange
  const TokenPreimage token_preimage(kTokenPreimageBase64);

  // Act
  std::stringstream ss;
  ss << token_preimage;

  // Assert
  EXPECT_EQ(kTokenPreimageBase64, ss.str());
}

TEST(BraveAdsTokenPreimageTest, OutputStreamWhenUninitialized) {
  // Arrange
  const TokenPreimage token_preimage;

  // Act
  std::stringstream ss;
  ss << token_preimage;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace brave_ads::privacy::cbr
