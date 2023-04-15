/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/dleq_proof.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signed_token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signing_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signing_key_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::privacy::cbr {

namespace {
constexpr char kDLEQProofBase64[] =
    R"(8vp0QItdO24oqOZB8m8rCB85VUftBhnpZ8kDovYP9AvvlaEpwDFbTi72B1ZEJmumS5TazlWlLBlI4HrWDCMvDg==)";
}  // namespace

TEST(BatAdsDLEQProofTest, FailToInitialize) {
  // Arrange
  const DLEQProof dleq_proof;

  // Act
  const bool has_value = dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsDLEQProofTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const DLEQProof dleq_proof("");

  // Act
  const bool has_value = dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsDLEQProofTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const DLEQProof dleq_proof(kInvalidBase64);

  // Act
  const bool has_value = dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsDLEQProofTest, FailToInitializeWithInvalidBlindedToken) {
  // Arrange
  const DLEQProof dleq_proof(GetInvalidBlindedToken(), GetSignedToken(),
                             GetSigningKey());

  // Act
  const bool has_value = dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsDLEQProofTest, FailToInitializeWithInvalidSignedToken) {
  // Arrange
  const DLEQProof dleq_proof(GetBlindedToken(), GetInvalidSignedToken(),
                             GetSigningKey());

  // Act
  const bool has_value = dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsDLEQProofTest, FailToInitializeWithInvalidSigningKey) {
  // Arrange
  const DLEQProof dleq_proof(GetBlindedToken(), GetSignedToken(),
                             GetInvalidSigningKey());

  // Act
  const bool has_value = dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsDLEQProofTest, DecodeBase64) {
  // Arrange

  // Act
  const DLEQProof dleq_proof = DLEQProof::DecodeBase64(kDLEQProofBase64);

  // Assert
  const bool has_value = dleq_proof.has_value();
  EXPECT_TRUE(has_value);
}

TEST(BatAdsDLEQProofTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const DLEQProof dleq_proof = DLEQProof::DecodeBase64({});

  // Assert
  const bool has_value = dleq_proof.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsDLEQProofTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const DLEQProof dleq_proof = DLEQProof::DecodeBase64(kInvalidBase64);

  // Assert
  const bool has_value = dleq_proof.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsDLEQProofTest, EncodeBase64) {
  // Arrange
  const DLEQProof dleq_proof(kDLEQProofBase64);

  // Act
  const absl::optional<std::string> encoded_base64 = dleq_proof.EncodeBase64();
  ASSERT_TRUE(encoded_base64);

  // Assert
  EXPECT_EQ(kDLEQProofBase64, *encoded_base64);
}

TEST(BatAdsDLEQProofTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const DLEQProof dleq_proof;

  // Act
  const absl::optional<std::string> encoded_base64 = dleq_proof.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64);
}

TEST(BatAdsDLEQProofTest, Verify) {
  // Arrange
  DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  EXPECT_TRUE(
      dleq_proof.Verify(GetBlindedToken(), GetSignedToken(), GetPublicKey()));
}

TEST(BatAdsDLEQProofTest, FailToVerifyWhenUninitialized) {
  // Arrange
  DLEQProof dleq_proof;

  // Act

  // Assert
  EXPECT_FALSE(
      dleq_proof.Verify(GetBlindedToken(), GetSignedToken(), GetPublicKey()));
}

TEST(BatAdsDLEQProofTest, FailToVerifyWithInvalidBlindedToken) {
  // Arrange
  DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.Verify(GetInvalidBlindedToken(), GetSignedToken(),
                                 GetPublicKey()));
}

TEST(BatAdsDLEQProofTest, FailToVerifyWithInvalidSignedToken) {
  // Arrange
  DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.Verify(GetBlindedToken(), GetInvalidSignedToken(),
                                 GetPublicKey()));
}

TEST(BatAdsDLEQProofTest, FailToVerifyWithMismatchingPublicKey) {
  // Arrange
  DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.Verify(GetBlindedToken(), GetSignedToken(),
                                 GetMismatchingPublicKey()));
}

TEST(BatAdsDLEQProofTest, FailToVerifyWithInvalidPublicKey) {
  // Arrange
  DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.Verify(GetBlindedToken(), GetSignedToken(),
                                 GetInvalidPublicKey()));
}

TEST(BatAdsDLEQProofTest, IsEqual) {
  // Arrange
  const DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  EXPECT_EQ(dleq_proof, dleq_proof);
}

TEST(BatAdsDLEQProofTest, IsEqualWhenUninitialized) {
  // Arrange
  const DLEQProof dleq_proof;

  // Act

  // Assert
  EXPECT_EQ(dleq_proof, dleq_proof);
}

TEST(BatAdsDLEQProofTest, IsEmptyBase64Equal) {
  // Arrange
  const DLEQProof dleq_proof("");

  // Act

  // Assert
  EXPECT_EQ(dleq_proof, dleq_proof);
}

TEST(BatAdsDLEQProofTest, IsInvalidBase64Equal) {
  // Arrange
  const DLEQProof dleq_proof(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(dleq_proof, dleq_proof);
}

TEST(BatAdsDLEQProofTest, IsNotEqual) {
  // Arrange
  const DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  const DLEQProof different_dleq_proof(kInvalidBase64);
  EXPECT_NE(different_dleq_proof, dleq_proof);
}

TEST(BatAdsDLEQProofTest, OutputStream) {
  // Arrange
  const DLEQProof dleq_proof(kDLEQProofBase64);

  // Act
  std::stringstream ss;
  ss << dleq_proof;

  // Assert
  EXPECT_EQ(kDLEQProofBase64, ss.str());
}

TEST(BatAdsDLEQProofTest, OutputStreamWhenUninitialized) {
  // Arrange
  const DLEQProof dleq_proof;

  // Act
  std::stringstream ss;
  ss << dleq_proof;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace brave_ads::privacy::cbr
