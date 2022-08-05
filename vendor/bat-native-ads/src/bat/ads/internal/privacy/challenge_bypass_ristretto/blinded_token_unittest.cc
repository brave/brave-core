/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"

#include <sstream>

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace privacy {
namespace cbr {

TEST(BatAdsBlindedTokenTest, FailToInitialize) {
  // Arrange
  BlindedToken blinded_token;

  // Act
  const bool has_value = blinded_token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBlindedTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  BlindedToken blinded_token("");

  // Act
  const bool has_value = blinded_token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBlindedTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  BlindedToken blinded_token(kInvalidBase64);

  // Act
  const bool has_value = blinded_token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBlindedTokenTest, DecodeBase64) {
  // Arrange

  // Act
  const BlindedToken blinded_token =
      BlindedToken::DecodeBase64(kBlindedTokenBase64);

  // Assert
  const bool has_value = blinded_token.has_value();
  EXPECT_TRUE(has_value);
}

TEST(BatAdsBlindedTokenTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const BlindedToken blinded_token = BlindedToken::DecodeBase64("");

  // Assert
  const bool has_value = blinded_token.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBlindedTokenTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const BlindedToken blinded_token = BlindedToken::DecodeBase64(kInvalidBase64);

  // Assert
  const bool has_value = blinded_token.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBlindedTokenTest, EncodeBase64) {
  // Arrange
  BlindedToken blinded_token(kBlindedTokenBase64);

  // Act
  const absl::optional<std::string> encoded_base64_optional =
      blinded_token.EncodeBase64();
  ASSERT_TRUE(encoded_base64_optional);

  const std::string& encoded_base64 = encoded_base64_optional.value();

  // Assert
  EXPECT_EQ(kBlindedTokenBase64, encoded_base64);
}

TEST(BatAdsBlindedTokenTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  BlindedToken blinded_token;

  // Act
  const absl::optional<std::string> encoded_base64_optional =
      blinded_token.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64_optional);
}

TEST(BatAdsBlindedTokenTest, IsEqual) {
  // Arrange
  BlindedToken blinded_token(kBlindedTokenBase64);

  // Act

  // Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST(BatAdsBlindedTokenTest, IsEqualWhenUninitialized) {
  // Arrange
  BlindedToken blinded_token;

  // Act

  // Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST(BatAdsBlindedTokenTest, IsEmptyBase64Equal) {
  // Arrange
  BlindedToken blinded_token("");

  // Act

  // Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST(BatAdsBlindedTokenTest, IsInvalidBase64Equal) {
  // Arrange
  BlindedToken blinded_token(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST(BatAdsBlindedTokenTest, IsNotEqual) {
  // Arrange

  // Act
  BlindedToken blinded_token(kBlindedTokenBase64);
  BlindedToken different_blinded_token(kInvalidBase64);

  // Assert
  EXPECT_NE(different_blinded_token, blinded_token);
}

TEST(BatAdsBlindedTokenTest, OutputStream) {
  // Arrange
  BlindedToken blinded_token(kBlindedTokenBase64);

  // Act
  std::stringstream ss;
  ss << blinded_token;

  // Assert
  EXPECT_EQ(kBlindedTokenBase64, ss.str());
}

TEST(BatAdsBlindedTokenTest, OutputStreamWhenUninitialized) {
  // Arrange
  BlindedToken blinded_token;

  // Act
  std::stringstream ss;
  ss << blinded_token;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
