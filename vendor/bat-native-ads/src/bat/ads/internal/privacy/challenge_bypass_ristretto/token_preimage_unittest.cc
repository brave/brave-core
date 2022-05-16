/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token_preimage.h"

#include <sstream>

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace privacy {
namespace cbr {

TEST(BatAdsTokenPreimageTest, FailToInitialize) {
  // Arrange
  TokenPreimage token_preimage;

  // Act
  const bool has_value = token_preimage.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsTokenPreimageTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  TokenPreimage token_preimage("");

  // Act
  const bool has_value = token_preimage.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsTokenPreimageTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  TokenPreimage token_preimage(kInvalidBase64);

  // Act
  const bool has_value = token_preimage.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsTokenPreimageTest, DecodeBase64) {
  // Arrange

  // Act
  const TokenPreimage token_preimage =
      TokenPreimage::DecodeBase64(kTokenPreimageBase64);

  // Assert
  const bool has_value = token_preimage.has_value();
  EXPECT_TRUE(has_value);
}

TEST(BatAdsTokenPreimageTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const TokenPreimage token_preimage = TokenPreimage::DecodeBase64("");

  // Assert
  const bool has_value = token_preimage.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsTokenPreimageTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const TokenPreimage token_preimage =
      TokenPreimage::DecodeBase64(kInvalidBase64);

  // Assert
  const bool has_value = token_preimage.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsTokenPreimageTest, EncodeBase64) {
  // Arrange
  TokenPreimage token_preimage(kTokenPreimageBase64);

  // Act
  const absl::optional<std::string> encoded_base64_optional =
      token_preimage.EncodeBase64();
  ASSERT_TRUE(encoded_base64_optional);

  const std::string& encoded_base64 = encoded_base64_optional.value();

  // Assert
  EXPECT_EQ(kTokenPreimageBase64, encoded_base64);
}

TEST(BatAdsTokenPreimageTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  TokenPreimage token_preimage;

  // Act
  const absl::optional<std::string> encoded_base64_optional =
      token_preimage.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64_optional);
}

TEST(BatAdsTokenPreimageTest, IsEqual) {
  // Arrange
  TokenPreimage token_preimage(kTokenPreimageBase64);

  // Act

  // Assert
  EXPECT_EQ(token_preimage, token_preimage);
}

TEST(BatAdsTokenPreimageTest, IsEqualWhenUninitialized) {
  // Arrange
  TokenPreimage token_preimage;

  // Act

  // Assert
  EXPECT_EQ(token_preimage, token_preimage);
}

TEST(BatAdsTokenPreimageTest, IsEmptyBase64Equal) {
  // Arrange
  TokenPreimage token_preimage("");

  // Act

  // Assert
  EXPECT_EQ(token_preimage, token_preimage);
}

TEST(BatAdsTokenPreimageTest, IsInvalidBase64Equal) {
  // Arrange
  TokenPreimage token_preimage(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(token_preimage, token_preimage);
}

TEST(BatAdsTokenPreimageTest, IsNotEqual) {
  // Arrange
  TokenPreimage token_preimage(kTokenPreimageBase64);

  // Act

  // Assert
  TokenPreimage different_token_preimage(kInvalidBase64);
  EXPECT_NE(different_token_preimage, token_preimage);
}

TEST(BatAdsTokenPreimageTest, OutputStream) {
  // Arrange
  TokenPreimage token_preimage(kTokenPreimageBase64);

  // Act
  std::stringstream ss;
  ss << token_preimage;

  // Assert
  EXPECT_EQ(kTokenPreimageBase64, ss.str());
}

TEST(BatAdsTokenPreimageTest, OutputStreamWhenUninitialized) {
  // Arrange
  TokenPreimage token_preimage;

  // Act
  std::stringstream ss;
  ss << token_preimage;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
