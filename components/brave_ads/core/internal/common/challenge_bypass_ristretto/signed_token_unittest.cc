/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSignedTokenTest : public test::TestBase {};

TEST_F(BraveAdsSignedTokenTest, FailToInitialize) {
  // Arrange
  const cbr::SignedToken signed_token;

  // Act & Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST_F(BraveAdsSignedTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const cbr::SignedToken signed_token("");

  // Act & Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST_F(BraveAdsSignedTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const cbr::SignedToken signed_token(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST_F(BraveAdsSignedTokenTest, DecodeBase64) {
  // Act
  const cbr::SignedToken signed_token =
      cbr::SignedToken::DecodeBase64(cbr::test::kSignedTokenBase64);

  // Assert
  EXPECT_TRUE(signed_token.has_value());
}

TEST_F(BraveAdsSignedTokenTest, FailToDecodeEmptyBase64) {
  // Act
  const cbr::SignedToken signed_token = cbr::SignedToken::DecodeBase64("");

  // Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST_F(BraveAdsSignedTokenTest, FailToDecodeInvalidBase64) {
  // Act
  const cbr::SignedToken signed_token =
      cbr::SignedToken::DecodeBase64(cbr::test::kInvalidBase64);

  // Assert
  EXPECT_FALSE(signed_token.has_value());
}

TEST_F(BraveAdsSignedTokenTest, EncodeBase64) {
  // Arrange
  const cbr::SignedToken signed_token(cbr::test::kSignedTokenBase64);

  // Act & Assert
  EXPECT_EQ(cbr::test::kSignedTokenBase64, signed_token.EncodeBase64());
}

TEST_F(BraveAdsSignedTokenTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const cbr::SignedToken signed_token;

  // Act & Assert
  EXPECT_FALSE(signed_token.EncodeBase64());
}

TEST_F(BraveAdsSignedTokenTest, IsEqual) {
  // Arrange
  const cbr::SignedToken signed_token(cbr::test::kSignedTokenBase64);

  // Act & Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST_F(BraveAdsSignedTokenTest, IsEqualWhenUninitialized) {
  // Arrange
  const cbr::SignedToken signed_token;

  // Act & Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST_F(BraveAdsSignedTokenTest, IsEmptyBase64Equal) {
  // Arrange
  const cbr::SignedToken signed_token("");

  // Act & Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST_F(BraveAdsSignedTokenTest, IsInvalidBase64Equal) {
  // Arrange
  const cbr::SignedToken signed_token(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_EQ(signed_token, signed_token);
}

TEST_F(BraveAdsSignedTokenTest, IsNotEqual) {
  // Arrange
  const cbr::SignedToken signed_token(cbr::test::kSignedTokenBase64);

  // Act & Assert
  const cbr::SignedToken another_signed_token(cbr::test::kInvalidBase64);
  EXPECT_NE(another_signed_token, signed_token);
}

TEST_F(BraveAdsSignedTokenTest, OutputStream) {
  // Arrange
  const cbr::SignedToken signed_token(cbr::test::kSignedTokenBase64);

  // Act
  std::stringstream ss;
  ss << signed_token;

  // Assert
  EXPECT_EQ(cbr::test::kSignedTokenBase64, ss.str());
}

TEST_F(BraveAdsSignedTokenTest, OutputStreamWhenUninitialized) {
  // Arrange
  const cbr::SignedToken signed_token;

  // Act
  std::stringstream ss;
  ss << signed_token;

  // Assert
  EXPECT_THAT(ss.str(), ::testing::IsEmpty());
}

}  // namespace brave_ads
