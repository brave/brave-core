/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/batch_dleq_proof.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signing_key.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signing_key_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsBatchDLEQProofTest : public test::TestBase {};

TEST_F(BraveAdsBatchDLEQProofTest, FailToInitialize) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof;

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof("");

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToInitializeWithInvalidBlindedTokens) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof(
      cbr::test::GetInvalidBlindedTokens(), cbr::test::GetSignedTokens(),
      cbr::test::GetSigningKey());

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToInitializeWithInvalidSignedTokens) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof(
      cbr::test::GetBlindedTokens(), cbr::test::GetInvalidSignedTokens(),
      cbr::test::GetSigningKey());

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToInitializeWithInvalidSigningKey) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof(cbr::test::GetBlindedTokens(),
                                             cbr::test::GetSignedTokens(),
                                             cbr::test::GetInvalidSigningKey());

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, DecodeBase64) {
  // Act
  const cbr::BatchDLEQProof batch_dleq_proof =
      cbr::BatchDLEQProof::DecodeBase64(cbr::test::kBatchDLEQProofBase64);

  // Assert
  EXPECT_TRUE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToDecodeEmptyBase64) {
  // Act
  const cbr::BatchDLEQProof batch_dleq_proof =
      cbr::BatchDLEQProof::DecodeBase64("");

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToDecodeInvalidBase64) {
  // Act
  const cbr::BatchDLEQProof batch_dleq_proof =
      cbr::BatchDLEQProof::DecodeBase64(cbr::test::kInvalidBase64);

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST_F(BraveAdsBatchDLEQProofTest, EncodeBase64) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act & Assert
  EXPECT_EQ(cbr::test::kBatchDLEQProofBase64, batch_dleq_proof.EncodeBase64());
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof;

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.EncodeBase64());
}

TEST_F(BraveAdsBatchDLEQProofTest, Verify) {
  // Arrange
  cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act & Assert
  EXPECT_TRUE(batch_dleq_proof.Verify(cbr::test::GetBlindedTokens(),
                                      cbr::test::GetSignedTokens(),
                                      cbr::test::GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyWhenUninitialized) {
  // Arrange
  cbr::BatchDLEQProof batch_dleq_proof;

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(cbr::test::GetBlindedTokens(),
                                       cbr::test::GetSignedTokens(),
                                       cbr::test::GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyWithInvalidBlindedTokens) {
  // Arrange
  cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(cbr::test::GetInvalidBlindedTokens(),
                                       cbr::test::GetSignedTokens(),
                                       cbr::test::GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyWithInvalidSignedTokens) {
  // Arrange
  cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(cbr::test::GetBlindedTokens(),
                                       cbr::test::GetInvalidSignedTokens(),
                                       cbr::test::GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyWithMismatchingPublicKey) {
  // Arrange
  cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(cbr::test::GetBlindedTokens(),
                                       cbr::test::GetSignedTokens(),
                                       cbr::test::GetMismatchingPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyWithInvalidPublicKey) {
  // Arrange
  cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(cbr::test::GetBlindedTokens(),
                                       cbr::test::GetSignedTokens(),
                                       cbr::test::GetInvalidPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, VerifyAndUnblind) {
  // Arrange
  cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act & Assert
  EXPECT_EQ(cbr::test::GetUnblindedTokens(),
            batch_dleq_proof.VerifyAndUnblind(
                cbr::test::GetTokens(), cbr::test::GetBlindedTokens(),
                cbr::test::GetSignedTokens(), cbr::test::GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyAndUnblindWhenUninitialized) {
  // Arrange
  cbr::BatchDLEQProof batch_dleq_proof;

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.VerifyAndUnblind(
      cbr::test::GetTokens(), cbr::test::GetBlindedTokens(),
      cbr::test::GetSignedTokens(), cbr::test::GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithInvalidTokens) {
  // Arrange
  cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.VerifyAndUnblind(
      cbr::test::GetInvalidTokens(), cbr::test::GetBlindedTokens(),
      cbr::test::GetSignedTokens(), cbr::test::GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest,
       FailToVerifyAndUnblindWithInvalidBlindedTokens) {
  // Arrange
  cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.VerifyAndUnblind(
      cbr::test::GetTokens(), cbr::test::GetInvalidBlindedTokens(),
      cbr::test::GetSignedTokens(), cbr::test::GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest,
       FailToVerifyAndUnblindWithInvalidSignedTokens) {
  // Arrange
  cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.VerifyAndUnblind(
      cbr::test::GetTokens(), cbr::test::GetBlindedTokens(),
      cbr::test::GetInvalidSignedTokens(), cbr::test::GetPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest,
       FailToVerifyAndUnblindWithMismatchingPublicKey) {
  // Arrange
  cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.VerifyAndUnblind(
      cbr::test::GetTokens(), cbr::test::GetBlindedTokens(),
      cbr::test::GetSignedTokens(), cbr::test::GetMismatchingPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithInvalidPublicKey) {
  // Arrange
  cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act & Assert
  EXPECT_FALSE(batch_dleq_proof.VerifyAndUnblind(
      cbr::test::GetTokens(), cbr::test::GetBlindedTokens(),
      cbr::test::GetSignedTokens(), cbr::test::GetInvalidPublicKey()));
}

TEST_F(BraveAdsBatchDLEQProofTest, IsEqual) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act & Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST_F(BraveAdsBatchDLEQProofTest, IsEqualWhenUninitialized) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof;

  // Act & Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST_F(BraveAdsBatchDLEQProofTest, IsEmptyBase64Equal) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof("");

  // Act & Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST_F(BraveAdsBatchDLEQProofTest, IsInvalidBase64Equal) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kInvalidBase64);

  // Act & Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST_F(BraveAdsBatchDLEQProofTest, IsNotEqual) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act & Assert
  const cbr::BatchDLEQProof another_batch_dleq_proof(cbr::test::kInvalidBase64);
  EXPECT_NE(another_batch_dleq_proof, batch_dleq_proof);
}

TEST_F(BraveAdsBatchDLEQProofTest, OutputStream) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof(cbr::test::kBatchDLEQProofBase64);

  // Act
  std::stringstream ss;
  ss << batch_dleq_proof;

  // Assert
  EXPECT_EQ(cbr::test::kBatchDLEQProofBase64, ss.str());
}

TEST_F(BraveAdsBatchDLEQProofTest, OutputStreamWhenUninitialized) {
  // Arrange
  const cbr::BatchDLEQProof batch_dleq_proof;

  // Act
  std::stringstream ss;
  ss << batch_dleq_proof;

  // Assert
  EXPECT_THAT(ss.str(), ::testing::IsEmpty());
}

}  // namespace brave_ads
