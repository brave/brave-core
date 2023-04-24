/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/batch_dleq_proof.h"

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
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy::cbr {

namespace {
constexpr char kBatchDLEQProofBase64[] =
    R"(y0a409PTX6g97xC0Xq8cCuY7ElLPaP+QJ6DaHNfqlQWizBYCSWdaleakKatkyNswfPmkQuhL7awmzQ0ygEUGDw==)";
}  // namespace

TEST(BraveAdsBatchDLEQProofTest, FailToInitialize) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof;

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST(BraveAdsBatchDLEQProofTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof("");

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST(BraveAdsBatchDLEQProofTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kInvalidBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST(BraveAdsBatchDLEQProofTest, FailToInitializeWithInvalidBlindedTokens) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(GetInvalidBlindedTokens(),
                                        GetSignedTokens(), GetSigningKey());

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST(BraveAdsBatchDLEQProofTest, FailToInitializeWithInvalidSignedTokens) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(
      GetBlindedTokens(), GetInvalidSignedTokens(), GetSigningKey());

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST(BraveAdsBatchDLEQProofTest, FailToInitializeWithInvalidSigningKey) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(GetBlindedTokens(), GetSignedTokens(),
                                        GetInvalidSigningKey());

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST(BraveAdsBatchDLEQProofTest, DecodeBase64) {
  // Arrange

  // Act
  const BatchDLEQProof batch_dleq_proof =
      BatchDLEQProof::DecodeBase64(kBatchDLEQProofBase64);

  // Assert
  EXPECT_TRUE(batch_dleq_proof.has_value());
}

TEST(BraveAdsBatchDLEQProofTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const BatchDLEQProof batch_dleq_proof = BatchDLEQProof::DecodeBase64("");

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST(BraveAdsBatchDLEQProofTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const BatchDLEQProof batch_dleq_proof =
      BatchDLEQProof::DecodeBase64(kInvalidBase64);

  // Assert
  EXPECT_FALSE(batch_dleq_proof.has_value());
}

TEST(BraveAdsBatchDLEQProofTest, EncodeBase64) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::string> encoded_base64 =
      batch_dleq_proof.EncodeBase64();
  ASSERT_TRUE(encoded_base64);

  // Assert
  EXPECT_EQ(kBatchDLEQProofBase64, *encoded_base64);
}

TEST(BraveAdsBatchDLEQProofTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof;

  // Act
  const absl::optional<std::string> encoded_base64 =
      batch_dleq_proof.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64);
}

TEST(BraveAdsBatchDLEQProofTest, Verify) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_TRUE(batch_dleq_proof.Verify(GetBlindedTokens(), GetSignedTokens(),
                                      GetPublicKey()));
}

TEST(BraveAdsBatchDLEQProofTest, FailToVerifyWhenUninitialized) {
  // Arrange
  BatchDLEQProof batch_dleq_proof;

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(GetBlindedTokens(), GetSignedTokens(),
                                       GetPublicKey()));
}

TEST(BraveAdsBatchDLEQProofTest, FailToVerifyWithInvalidBlindedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(GetInvalidBlindedTokens(),
                                       GetSignedTokens(), GetPublicKey()));
}

TEST(BraveAdsBatchDLEQProofTest, FailToVerifyWithInvalidSignedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(
      GetBlindedTokens(), GetInvalidSignedTokens(), GetPublicKey()));
}

TEST(BraveAdsBatchDLEQProofTest, FailToVerifyWithMismatchingPublicKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(GetBlindedTokens(), GetSignedTokens(),
                                       GetMismatchingPublicKey()));
}

TEST(BraveAdsBatchDLEQProofTest, FailToVerifyWithInvalidPublicKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(GetBlindedTokens(), GetSignedTokens(),
                                       GetInvalidPublicKey()));
}

TEST(BraveAdsBatchDLEQProofTest, VerifyAndUnblind) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetBlindedTokens(),
                                        GetSignedTokens(), GetPublicKey());
  ASSERT_TRUE(unblinded_tokens);

  // Assert
  EXPECT_EQ(GetUnblindedTokens(), *unblinded_tokens);
}

TEST(BraveAdsBatchDLEQProofTest, FailToVerifyAndUnblindWhenUninitialized) {
  // Arrange
  BatchDLEQProof batch_dleq_proof;

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetBlindedTokens(),
                                        GetSignedTokens(), GetPublicKey());

  // Assert
  ASSERT_FALSE(unblinded_tokens);
}

TEST(BraveAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithInvalidTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(GetInvalidTokens(), GetBlindedTokens(),
                                        GetSignedTokens(), GetPublicKey());

  // Assert
  EXPECT_FALSE(unblinded_tokens);
}

TEST(BraveAdsBatchDLEQProofTest,
     FailToVerifyAndUnblindWithInvalidBlindedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetInvalidBlindedTokens(),
                                        GetSignedTokens(), GetPublicKey());

  // Assert
  EXPECT_FALSE(unblinded_tokens);
}

TEST(BraveAdsBatchDLEQProofTest,
     FailToVerifyAndUnblindWithInvalidSignedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetBlindedTokens(),
                                        GetInvalidSignedTokens(),
                                        GetPublicKey());

  // Assert
  EXPECT_FALSE(unblinded_tokens);
}

TEST(BraveAdsBatchDLEQProofTest,
     FailToVerifyAndUnblindWithMismatchingPublicKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetBlindedTokens(),
                                        GetSignedTokens(),
                                        GetMismatchingPublicKey());

  // Assert
  EXPECT_FALSE(unblinded_tokens);
}

TEST(BraveAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithInvalidPublicKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetBlindedTokens(),
                                        GetSignedTokens(),
                                        GetInvalidPublicKey());

  // Assert
  EXPECT_FALSE(unblinded_tokens);
}

TEST(BraveAdsBatchDLEQProofTest, IsEqual) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST(BraveAdsBatchDLEQProofTest, IsEqualWhenUninitialized) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof;

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST(BraveAdsBatchDLEQProofTest, IsEmptyBase64Equal) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof("");

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST(BraveAdsBatchDLEQProofTest, IsInvalidBase64Equal) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST(BraveAdsBatchDLEQProofTest, IsNotEqual) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  const BatchDLEQProof different_batch_dleq_proof(kInvalidBase64);
  EXPECT_NE(different_batch_dleq_proof, batch_dleq_proof);
}

TEST(BraveAdsBatchDLEQProofTest, OutputStream) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  std::stringstream ss;
  ss << batch_dleq_proof;

  // Assert
  EXPECT_EQ(kBatchDLEQProofBase64, ss.str());
}

TEST(BraveAdsBatchDLEQProofTest, OutputStreamWhenUninitialized) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof;

  // Act
  std::stringstream ss;
  ss << batch_dleq_proof;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace brave_ads::privacy::cbr
