/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_preimage.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTokenPreimageTest : public test::TestBase {};

TEST_F(BraveAdsTokenPreimageTest, FailToInitialize) {
  // Arrange
  const cbr::TokenPreimage token_preimage;

  // Act & Assert
  EXPECT_FALSE(token_preimage.has_value());
}

TEST_F(BraveAdsTokenPreimageTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const cbr::TokenPreimage token_preimage("");

  // Act & Assert
  EXPECT_FALSE(token_preimage.has_value());
}

TEST_F(BraveAdsTokenPreimageTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const cbr::TokenPreimage token_preimage(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(token_preimage.has_value());
}

TEST_F(BraveAdsTokenPreimageTest, DecodeBase64) {
  // Act
  const cbr::TokenPreimage token_preimage =
      cbr::TokenPreimage::DecodeBase64(cbr::test::kTokenPreimageBase64);

  // Assert
  EXPECT_TRUE(token_preimage.has_value());
}

TEST_F(BraveAdsTokenPreimageTest, FailToDecodeEmptyBase64) {
  // Act
  const cbr::TokenPreimage token_preimage =
      cbr::TokenPreimage::DecodeBase64("");

  // Assert
  EXPECT_FALSE(token_preimage.has_value());
}

TEST_F(BraveAdsTokenPreimageTest, FailToDecodeInvalidBase64) {
  // Act
  const cbr::TokenPreimage token_preimage =
      cbr::TokenPreimage::DecodeBase64(cbr::test::kInvalidBase64);

  // Assert
  EXPECT_FALSE(token_preimage.has_value());
}

TEST_F(BraveAdsTokenPreimageTest, EncodeBase64) {
  // Arrange
  const cbr::TokenPreimage token_preimage(cbr::test::kTokenPreimageBase64);

  // Act & Assert
  EXPECT_EQ(cbr::test::kTokenPreimageBase64, token_preimage.EncodeBase64());
}

TEST_F(BraveAdsTokenPreimageTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const cbr::TokenPreimage token_preimage;

  // Act & Assert
  EXPECT_FALSE(token_preimage.EncodeBase64());
}

TEST_F(BraveAdsTokenPreimageTest, IsEqual) {
  // Arrange
  const cbr::TokenPreimage token_preimage(cbr::test::kTokenPreimageBase64);

  // Act & Assert
  EXPECT_EQ(token_preimage, token_preimage);
}

TEST_F(BraveAdsTokenPreimageTest, IsEqualWhenUninitialized) {
  // Arrange
  const cbr::TokenPreimage token_preimage;

  // Act & Assert
  EXPECT_EQ(token_preimage, token_preimage);
}

TEST_F(BraveAdsTokenPreimageTest, IsEmptyBase64Equal) {
  // Arrange
  const cbr::TokenPreimage token_preimage("");

  // Act & Assert
  EXPECT_EQ(token_preimage, token_preimage);
}

TEST_F(BraveAdsTokenPreimageTest, IsInvalidBase64Equal) {
  // Arrange
  const cbr::TokenPreimage token_preimage(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_EQ(token_preimage, token_preimage);
}

TEST_F(BraveAdsTokenPreimageTest, IsNotEqual) {
  // Arrange
  const cbr::TokenPreimage token_preimage(cbr::test::kTokenPreimageBase64);

  // Act & Assert
  const cbr::TokenPreimage another_token_preimage(cbr::test::kInvalidBase64);
  EXPECT_NE(another_token_preimage, token_preimage);
}

TEST_F(BraveAdsTokenPreimageTest, OutputStream) {
  // Arrange
  const cbr::TokenPreimage token_preimage(cbr::test::kTokenPreimageBase64);

  // Act
  std::stringstream ss;
  ss << token_preimage;

  // Assert
  EXPECT_EQ(cbr::test::kTokenPreimageBase64, ss.str());
}

TEST_F(BraveAdsTokenPreimageTest, OutputStreamWhenUninitialized) {
  // Arrange
  const cbr::TokenPreimage token_preimage;

  // Act
  std::stringstream ss;
  ss << token_preimage;

  // Assert
  EXPECT_THAT(ss.str(), ::testing::IsEmpty());
}

}  // namespace brave_ads
