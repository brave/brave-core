/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsBlindedTokenTest : public test::TestBase {};

TEST_F(BraveAdsBlindedTokenTest, FailToInitialize) {
  // Arrange
  const cbr::BlindedToken blinded_token;

  // Act & Assert
  EXPECT_FALSE(blinded_token.has_value());
}

TEST_F(BraveAdsBlindedTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const cbr::BlindedToken blinded_token("");

  // Act & Assert
  EXPECT_FALSE(blinded_token.has_value());
}

TEST_F(BraveAdsBlindedTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const cbr::BlindedToken blinded_token(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(blinded_token.has_value());
}

TEST_F(BraveAdsBlindedTokenTest, DecodeBase64) {
  // Act
  const cbr::BlindedToken blinded_token =
      cbr::BlindedToken::DecodeBase64(cbr::test::kBlindedTokenBase64);

  // Assert
  EXPECT_TRUE(blinded_token.has_value());
}

TEST_F(BraveAdsBlindedTokenTest, FailToDecodeEmptyBase64) {
  // Act
  const cbr::BlindedToken blinded_token = cbr::BlindedToken::DecodeBase64("");

  // Assert
  EXPECT_FALSE(blinded_token.has_value());
}

TEST_F(BraveAdsBlindedTokenTest, FailToDecodeInvalidBase64) {
  // Act
  const cbr::BlindedToken blinded_token =
      cbr::BlindedToken::DecodeBase64(cbr::test::kInvalidBase64);

  // Assert
  EXPECT_FALSE(blinded_token.has_value());
}

TEST_F(BraveAdsBlindedTokenTest, EncodeBase64) {
  // Arrange
  const cbr::BlindedToken blinded_token(cbr::test::kBlindedTokenBase64);

  // Act & Assert
  EXPECT_EQ(cbr::test::kBlindedTokenBase64, blinded_token.EncodeBase64());
}

TEST_F(BraveAdsBlindedTokenTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const cbr::BlindedToken blinded_token;

  // Act & Assert
  EXPECT_FALSE(blinded_token.EncodeBase64());
}

TEST_F(BraveAdsBlindedTokenTest, IsEqual) {
  // Arrange
  const cbr::BlindedToken blinded_token(cbr::test::kBlindedTokenBase64);

  // Act & Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST_F(BraveAdsBlindedTokenTest, IsEqualWhenUninitialized) {
  // Arrange
  const cbr::BlindedToken blinded_token;

  // Act & Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST_F(BraveAdsBlindedTokenTest, IsEmptyBase64Equal) {
  // Arrange
  const cbr::BlindedToken blinded_token("");

  // Act & Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST_F(BraveAdsBlindedTokenTest, IsInvalidBase64Equal) {
  // Arrange
  const cbr::BlindedToken blinded_token(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_EQ(blinded_token, blinded_token);
}

TEST_F(BraveAdsBlindedTokenTest, IsNotEqual) {
  // Act & Assert
  EXPECT_NE(cbr::BlindedToken(cbr::test::kBlindedTokenBase64),
            cbr::BlindedToken(cbr::test::kInvalidBase64));
}

TEST_F(BraveAdsBlindedTokenTest, OutputStream) {
  // Arrange
  const cbr::BlindedToken blinded_token(cbr::test::kBlindedTokenBase64);

  // Act
  std::stringstream ss;
  ss << blinded_token;

  // Assert
  EXPECT_EQ(cbr::test::kBlindedTokenBase64, ss.str());
}

TEST_F(BraveAdsBlindedTokenTest, OutputStreamWhenUninitialized) {
  // Arrange
  const cbr::BlindedToken blinded_token;

  // Act
  std::stringstream ss;
  ss << blinded_token;

  // Assert
  EXPECT_THAT(ss.str(), ::testing::IsEmpty());
}

}  // namespace brave_ads
