/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/batch_dleq_proof.h"

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
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token_unittest_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy::cbr {

namespace {
constexpr char kBatchDLEQProofBase64[] =
    R"(y0a409PTX6g97xC0Xq8cCuY7ElLPaP+QJ6DaHNfqlQWizBYCSWdaleakKatkyNswfPmkQuhL7awmzQ0ygEUGDw==)";
}  // namespace

class BraveAdsBatchDLEQProofTest : public UnitTestBase {};

TEST_F(BraveAdsBatchDLEQProofTest, FailToInitialize) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof;

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof("");

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kInvalidBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToInitializeWithInvalidBlindedTokens) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(GetInvalidBlindedTokens(),
                                        GetSignedTokens(), GetSigningKey());

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToInitializeWithInvalidSignedTokens) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(
      GetBlindedTokens(), GetInvalidSignedTokens(), GetSigningKey());

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToInitializeWithInvalidSigningKey) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(GetBlindedTokens(), GetSignedTokens(),
                                        GetInvalidSigningKey());

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, DecodeBase64) {
  // Arrange

  // Act
  const BatchDLEQProof batch_dleq_proof =
      BatchDLEQProof::DecodeBase64(kBatchDLEQProofBase64);

  // Assert
  EXPECT_TRUE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const BatchDLEQProof batch_dleq_proof = BatchDLEQProof::DecodeBase64("");

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const BatchDLEQProof batch_dleq_proof =
      BatchDLEQProof::DecodeBase64(kInvalidBase64);

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, EncodeBase64) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_EQ(kBatchDLEQProofBase64, batch_dleq_proof.EncodeBase64());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof;

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.EncodeBase64());
}

TEST_F(BraveAdsBatchDLEQProofTest, Verify) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_TRUE(batch_dleq_proof.Verify(GetBlindedTokens(), GetSignedTokens(),
                                      GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyWhenUninitialized) {
  // Arrange
  BatchDLEQProof batch_dleq_proof;

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(GetBlindedTokens(), GetSignedTokens(),
                                       GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyWithInvalidBlindedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(GetInvalidBlindedTokens(),
                                       GetSignedTokens(), GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyWithInvalidSignedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(
      GetBlindedTokens(), GetInvalidSignedTokens(), GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyWithMismatchingPublicKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(GetBlindedTokens(), GetSignedTokens(),
                                       GetMismatchingPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyWithInvalidPublicKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(GetBlindedTokens(), GetSignedTokens(),
                                       GetInvalidPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, VerifyAndUnblind) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_EQ(GetUnblindedTokens(), batch_dleq_proof.VerifyAndUnblind(
                                      GetTokens(), GetBlindedTokens(),
                                      GetSignedTokens(), GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyAndUnblindWhenUninitialized) {
  // Arrange
  BatchDLEQProof batch_dleq_proof;

  // Act

  // Assert
  ASSERT_FALSE(batch_dleq_proof.VerifyAndUnblind(
      GetTokens(), GetBlindedTokens(), GetSignedTokens(), GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithInvalidTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(
      batch_dleq_proof.VerifyAndUnblind(GetInvalidTokens(), GetBlindedTokens(),
                                        GetSignedTokens(), GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest,
       FailToVerifyAndUnblindWithInvalidBlindedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetInvalidBlindedTokens(),
                                        GetSignedTokens(), GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest,
       FailToVerifyAndUnblindWithInvalidSignedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.VerifyAndUnblind(
      GetTokens(), GetBlindedTokens(), GetInvalidSignedTokens(),
      GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest,
       FailToVerifyAndUnblindWithMismatchingPublicKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.VerifyAndUnblind(
      GetTokens(), GetBlindedTokens(), GetSignedTokens(),
      GetMismatchingPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithInvalidPublicKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.VerifyAndUnblind(
      GetTokens(), GetBlindedTokens(), GetSignedTokens(),
      GetInvalidPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, IsEqual) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST_F(BraveAdsBatchDLEQProofTest, IsEqualWhenUninitialized) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof;

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST_F(BraveAdsBatchDLEQProofTest, IsEmptyBase64Equal) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof("");

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST_F(BraveAdsBatchDLEQProofTest, IsInvalidBase64Equal) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST_F(BraveAdsBatchDLEQProofTest, IsNotEqual) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  const BatchDLEQProof different_batch_dleq_proof(kInvalidBase64);
  EXPECT_NE(different_batch_dleq_proof, batch_dleq_proof);
}

TEST_F(BraveAdsBatchDLEQProofTest, OutputStream) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  std::stringstream ss;
  ss << batch_dleq_proof;

  // Assert
  EXPECT_EQ(kBatchDLEQProofBase64, ss.str());
}

TEST_F(BraveAdsBatchDLEQProofTest, OutputStreamWhenUninitialized) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof;

  // Act
  std::stringstream ss;
  ss << batch_dleq_proof;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace brave_ads::privacy::cbr
