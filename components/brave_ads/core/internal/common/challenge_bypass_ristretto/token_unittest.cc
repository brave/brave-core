/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTokenTest : public test::TestBase {};

TEST_F(BraveAdsTokenTest, Random) {
  // Arrange
  const cbr::Token token;

  // Act & Assert
  EXPECT_TRUE(token.has_value());
}

TEST_F(BraveAdsTokenTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const cbr::Token token("");

  // Act & Assert
  EXPECT_FALSE(token.has_value());
}

TEST_F(BraveAdsTokenTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const cbr::Token token(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(token.has_value());
}

TEST_F(BraveAdsTokenTest, DecodeBase64) {
  // Act
  const cbr::Token token = cbr::Token::DecodeBase64(cbr::test::kTokenBase64);

  // Assert
  EXPECT_TRUE(token.has_value());
}

TEST_F(BraveAdsTokenTest, FailToDecodeEmptyBase64) {
  // Act
  const cbr::Token token = cbr::Token::DecodeBase64("");

  // Assert
  EXPECT_FALSE(token.has_value());
}

TEST_F(BraveAdsTokenTest, FailToDecodeInvalidBase64) {
  // Act
  const cbr::Token token = cbr::Token::DecodeBase64(cbr::test::kInvalidBase64);

  // Assert
  EXPECT_FALSE(token.has_value());
}

TEST_F(BraveAdsTokenTest, EncodeBase64) {
  // Arrange
  const cbr::Token token(cbr::test::kTokenBase64);

  // Act & Assert
  EXPECT_EQ(cbr::test::kTokenBase64, token.EncodeBase64());
}

TEST_F(BraveAdsTokenTest, EncodeRandomBase64) {
  // Arrange
  const cbr::Token token;

  // Act & Assert
  EXPECT_TRUE(token.EncodeBase64());
}

TEST_F(BraveAdsTokenTest, IsEqual) {
  // Arrange
  const cbr::Token token;

  // Act & Assert
  EXPECT_EQ(token, token);
}

TEST_F(BraveAdsTokenTest, IsEmptyBase64Equal) {
  // Arrange
  const cbr::Token token("");

  // Act & Assert
  EXPECT_EQ(token, token);
}

TEST_F(BraveAdsTokenTest, IsInvalidBase64Equal) {
  // Arrange
  const cbr::Token token(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_EQ(token, token);
}

TEST_F(BraveAdsTokenTest, IsNotEqual) {
  // Arrange
  const cbr::Token token;

  // Act & Assert
  const cbr::Token another_token;
  EXPECT_NE(another_token, token);
}

TEST_F(BraveAdsTokenTest, OutputStream) {
  // Arrange
  const cbr::Token token(cbr::test::kTokenBase64);

  // Act
  std::stringstream ss;
  ss << token;

  // Assert
  EXPECT_EQ(cbr::test::kTokenBase64, ss.str());
}

}  // namespace brave_ads
