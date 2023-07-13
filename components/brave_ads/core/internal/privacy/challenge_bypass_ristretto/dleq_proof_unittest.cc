/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/dleq_proof.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signed_token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signing_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signing_key_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy::cbr {

namespace {
constexpr char kDLEQProofBase64[] =
    R"(8vp0QItdO24oqOZB8m8rCB85VUftBhnpZ8kDovYP9AvvlaEpwDFbTi72B1ZEJmumS5TazlWlLBlI4HrWDCMvDg==)";
}  // namespace

class BraveAdsDLEQProofTest : public UnitTestBase {};

TEST_F(BraveAdsDLEQProofTest, FailToInitialize) {
  // Arrange
  const DLEQProof dleq_proof;

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.has_value());
}

TEST_F(BraveAdsDLEQProofTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const DLEQProof dleq_proof("");

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.has_value());
}

TEST_F(BraveAdsDLEQProofTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const DLEQProof dleq_proof(kInvalidBase64);

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.has_value());
}

TEST_F(BraveAdsDLEQProofTest, FailToInitializeWithInvalidBlindedToken) {
  // Arrange
  const DLEQProof dleq_proof(GetInvalidBlindedToken(), GetSignedToken(),
                             GetSigningKey());

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.has_value());
}

TEST_F(BraveAdsDLEQProofTest, FailToInitializeWithInvalidSignedToken) {
  // Arrange
  const DLEQProof dleq_proof(GetBlindedToken(), GetInvalidSignedToken(),
                             GetSigningKey());

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.has_value());
}

TEST_F(BraveAdsDLEQProofTest, FailToInitializeWithInvalidSigningKey) {
  // Arrange
  const DLEQProof dleq_proof(GetBlindedToken(), GetSignedToken(),
                             GetInvalidSigningKey());

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.has_value());
}

TEST_F(BraveAdsDLEQProofTest, DecodeBase64) {
  // Arrange

  // Act
  const DLEQProof dleq_proof = DLEQProof::DecodeBase64(kDLEQProofBase64);

  // Assert
  EXPECT_TRUE(dleq_proof.has_value());
}

TEST_F(BraveAdsDLEQProofTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const DLEQProof dleq_proof = DLEQProof::DecodeBase64("");

  // Assert
  EXPECT_FALSE(dleq_proof.has_value());
}

TEST_F(BraveAdsDLEQProofTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const DLEQProof dleq_proof = DLEQProof::DecodeBase64(kInvalidBase64);

  // Assert
  EXPECT_FALSE(dleq_proof.has_value());
}

TEST_F(BraveAdsDLEQProofTest, EncodeBase64) {
  // Arrange
  const DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  EXPECT_EQ(kDLEQProofBase64, dleq_proof.EncodeBase64());
}

TEST_F(BraveAdsDLEQProofTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const DLEQProof dleq_proof;

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.EncodeBase64());
}

TEST_F(BraveAdsDLEQProofTest, Verify) {
  // Arrange
  DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  EXPECT_TRUE(
      dleq_proof.Verify(GetBlindedToken(), GetSignedToken(), GetPublicKey()));
}

TEST_F(BraveAdsDLEQProofTest, FailToVerifyWhenUninitialized) {
  // Arrange
  DLEQProof dleq_proof;

  // Act

  // Assert
  EXPECT_FALSE(
      dleq_proof.Verify(GetBlindedToken(), GetSignedToken(), GetPublicKey()));
}

TEST_F(BraveAdsDLEQProofTest, FailToVerifyWithInvalidBlindedToken) {
  // Arrange
  DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.Verify(GetInvalidBlindedToken(), GetSignedToken(),
                                 GetPublicKey()));
}

TEST_F(BraveAdsDLEQProofTest, FailToVerifyWithInvalidSignedToken) {
  // Arrange
  DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.Verify(GetBlindedToken(), GetInvalidSignedToken(),
                                 GetPublicKey()));
}

TEST_F(BraveAdsDLEQProofTest, FailToVerifyWithMismatchingPublicKey) {
  // Arrange
  DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.Verify(GetBlindedToken(), GetSignedToken(),
                                 GetMismatchingPublicKey()));
}

TEST_F(BraveAdsDLEQProofTest, FailToVerifyWithInvalidPublicKey) {
  // Arrange
  DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(dleq_proof.Verify(GetBlindedToken(), GetSignedToken(),
                                 GetInvalidPublicKey()));
}

TEST_F(BraveAdsDLEQProofTest, IsEqual) {
  // Arrange
  const DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  EXPECT_EQ(dleq_proof, dleq_proof);
}

TEST_F(BraveAdsDLEQProofTest, IsEqualWhenUninitialized) {
  // Arrange
  const DLEQProof dleq_proof;

  // Act

  // Assert
  EXPECT_EQ(dleq_proof, dleq_proof);
}

TEST_F(BraveAdsDLEQProofTest, IsEmptyBase64Equal) {
  // Arrange
  const DLEQProof dleq_proof("");

  // Act

  // Assert
  EXPECT_EQ(dleq_proof, dleq_proof);
}

TEST_F(BraveAdsDLEQProofTest, IsInvalidBase64Equal) {
  // Arrange
  const DLEQProof dleq_proof(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(dleq_proof, dleq_proof);
}

TEST_F(BraveAdsDLEQProofTest, IsNotEqual) {
  // Arrange
  const DLEQProof dleq_proof(kDLEQProofBase64);

  // Act

  // Assert
  const DLEQProof different_dleq_proof(kInvalidBase64);
  EXPECT_NE(different_dleq_proof, dleq_proof);
}

TEST_F(BraveAdsDLEQProofTest, OutputStream) {
  // Arrange
  const DLEQProof dleq_proof(kDLEQProofBase64);

  // Act
  std::stringstream ss;
  ss << dleq_proof;

  // Assert
  EXPECT_EQ(kDLEQProofBase64, ss.str());
}

TEST_F(BraveAdsDLEQProofTest, OutputStreamWhenUninitialized) {
  // Arrange
  const DLEQProof dleq_proof;

  // Act
  std::stringstream ss;
  ss << dleq_proof;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace brave_ads::privacy::cbr
