/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy::cbr {

TEST(BraveAdsBlindedTokenTest, FailToInitialize) {
  // Arrange
  const BlindedToken blinded_token;

  // Act
  const bool has_value = blinded_token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BraveAdsBlindedTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const BlindedToken blinded_token("");

  // Act
  const bool has_value = blinded_token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BraveAdsBlindedTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const BlindedToken blinded_token(kInvalidBase64);

  // Act
  const bool has_value = blinded_token.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BraveAdsBlindedTokenTest, DecodeBase64) {
  // Arrange

  // Act
  const BlindedToken blinded_token =
      BlindedToken::DecodeBase64(kBlindedTokenBase64);

  // Assert
  const bool has_value = blinded_token.has_value();
  EXPECT_TRUE(has_value);
}

TEST(BraveAdsBlindedTokenTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const BlindedToken blinded_token = BlindedToken::DecodeBase64({});

  // Assert
  const bool has_value = blinded_token.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BraveAdsBlindedTokenTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const BlindedToken blinded_token = BlindedToken::DecodeBase64(kInvalidBase64);

  // Assert
  const bool has_value = blinded_token.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BraveAdsBlindedTokenTest, EncodeBase64) {
  // Arrange
  const BlindedToken blinded_token(kBlindedTokenBase64);

  // Act
  const absl::optional<std::string> encoded_base64 =
      blinded_token.EncodeBase64();
  ASSERT_TRUE(encoded_base64);

  // Assert
  EXPECT_EQ(kBlindedTokenBase64, *encoded_base64);
}

TEST(BraveAdsBlindedTokenTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const BlindedToken blinded_token;

  // Act
  const absl::optional<std::string> encoded_base64 =
      blinded_token.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64);
}

TEST(BraveAdsBlindedTokenTest, IsEqual) {
  // Arrange
  const BlindedToken blinded_token(kBlindedTokenBase64);

  // Act

  // Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST(BraveAdsBlindedTokenTest, IsEqualWhenUninitialized) {
  // Arrange
  const BlindedToken blinded_token;

  // Act

  // Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST(BraveAdsBlindedTokenTest, IsEmptyBase64Equal) {
  // Arrange
  const BlindedToken blinded_token("");

  // Act

  // Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST(BraveAdsBlindedTokenTest, IsInvalidBase64Equal) {
  // Arrange
  const BlindedToken blinded_token(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST(BraveAdsBlindedTokenTest, IsNotEqual) {
  // Arrange

  // Act
  const BlindedToken blinded_token(kBlindedTokenBase64);
  const BlindedToken different_blinded_token(kInvalidBase64);

  // Assert
  EXPECT_NE(different_blinded_token, blinded_token);
}

TEST(BraveAdsBlindedTokenTest, OutputStream) {
  // Arrange
  const BlindedToken blinded_token(kBlindedTokenBase64);

  // Act
  std::stringstream ss;
  ss << blinded_token;

  // Assert
  EXPECT_EQ(kBlindedTokenBase64, ss.str());
}

TEST(BraveAdsBlindedTokenTest, OutputStreamWhenUninitialized) {
  // Arrange
  const BlindedToken blinded_token;

  // Act
  std::stringstream ss;
  ss << blinded_token;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace brave_ads::privacy::cbr
