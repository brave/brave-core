/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::cbr {

class BraveAdsBlindedTokenTest : public UnitTestBase {};

TEST_F(BraveAdsBlindedTokenTest, FailToInitialize) {
  // Arrange
  const BlindedToken blinded_token;

  // Act & Assert
  EXPECT_FALSE(blinded_token.has_value());
}

TEST_F(BraveAdsBlindedTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const BlindedToken blinded_token("");

  // Act & Assert
  EXPECT_FALSE(blinded_token.has_value());
}

TEST_F(BraveAdsBlindedTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const BlindedToken blinded_token(kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(blinded_token.has_value());
}

TEST_F(BraveAdsBlindedTokenTest, DecodeBase64) {
  // Act
  const BlindedToken blinded_token =
      BlindedToken::DecodeBase64(kBlindedTokenBase64);

  // Assert
  EXPECT_TRUE(blinded_token.has_value());
}

TEST_F(BraveAdsBlindedTokenTest, FailToDecodeEmptyBase64) {
  // Act
  const BlindedToken blinded_token = BlindedToken::DecodeBase64("");

  // Assert
  EXPECT_FALSE(blinded_token.has_value());
}

TEST_F(BraveAdsBlindedTokenTest, FailToDecodeInvalidBase64) {
  // Act
  const BlindedToken blinded_token = BlindedToken::DecodeBase64(kInvalidBase64);

  // Assert
  EXPECT_FALSE(blinded_token.has_value());
}

TEST_F(BraveAdsBlindedTokenTest, EncodeBase64) {
  // Arrange
  const BlindedToken blinded_token(kBlindedTokenBase64);

  // Act & Assert
  EXPECT_EQ(kBlindedTokenBase64, blinded_token.EncodeBase64());
}

TEST_F(BraveAdsBlindedTokenTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const BlindedToken blinded_token;

  // Act & Assert
  EXPECT_FALSE(blinded_token.EncodeBase64());
}

TEST_F(BraveAdsBlindedTokenTest, IsEqual) {
  // Arrange
  const BlindedToken blinded_token(kBlindedTokenBase64);

  // Act & Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST_F(BraveAdsBlindedTokenTest, IsEqualWhenUninitialized) {
  // Arrange
  const BlindedToken blinded_token;

  // Act & Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST_F(BraveAdsBlindedTokenTest, IsEmptyBase64Equal) {
  // Arrange
  const BlindedToken blinded_token("");

  // Act & Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST_F(BraveAdsBlindedTokenTest, IsInvalidBase64Equal) {
  // Arrange
  const BlindedToken blinded_token(kInvalidBase64);

  // Act & Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST_F(BraveAdsBlindedTokenTest, IsNotEqual) {
  // Act & Assert
  EXPECT_NE(BlindedToken(kBlindedTokenBase64), BlindedToken(kInvalidBase64));
}

TEST_F(BraveAdsBlindedTokenTest, OutputStream) {
  // Arrange
  const BlindedToken blinded_token(kBlindedTokenBase64);

  // Act
  std::stringstream ss;
  ss << blinded_token;

  // Assert
  EXPECT_EQ(kBlindedTokenBase64, ss.str());
}

TEST_F(BraveAdsBlindedTokenTest, OutputStreamWhenUninitialized) {
  // Arrange
  const BlindedToken blinded_token;

  // Act
  std::stringstream ss;
  ss << blinded_token;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace brave_ads::cbr
