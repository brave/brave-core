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

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::privacy::cbr {

namespace {
constexpr char kBatchDLEQProofBase64[] =
    R"(y0a409PTX6g97xC0Xq8cCuY7ElLPaP+QJ6DaHNfqlQWizBYCSWdaleakKatkyNswfPmkQuhL7awmzQ0ygEUGDw==)";
}  // namespace

TEST(BatAdsBatchDLEQProofTest, FailToInitialize) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof;

  // Act
  const bool has_value = batch_dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBatchDLEQProofTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof("");

  // Act
  const bool has_value = batch_dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBatchDLEQProofTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kInvalidBase64);

  // Act
  const bool has_value = batch_dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBatchDLEQProofTest, FailToInitializeWithInvalidBlindedTokens) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(GetInvalidBlindedTokens(),
                                        GetSignedTokens(), GetSigningKey());

  // Act
  const bool has_value = batch_dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBatchDLEQProofTest, FailToInitializeWithInvalidSignedTokens) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(
      GetBlindedTokens(), GetInvalidSignedTokens(), GetSigningKey());

  // Act
  const bool has_value = batch_dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBatchDLEQProofTest, FailToInitializeWithInvalidSigningKey) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(GetBlindedTokens(), GetSignedTokens(),
                                        GetInvalidSigningKey());

  // Act
  const bool has_value = batch_dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBatchDLEQProofTest, DecodeBase64) {
  // Arrange

  // Act
  const BatchDLEQProof batch_dleq_proof =
      BatchDLEQProof::DecodeBase64(kBatchDLEQProofBase64);

  // Assert
  const bool has_value = batch_dleq_proof.has_value();
  EXPECT_TRUE(has_value);
}

TEST(BatAdsBatchDLEQProofTest, FailToDecodeEmptyBase64) {
  // Arrange

  // Act
  const BatchDLEQProof batch_dleq_proof = BatchDLEQProof::DecodeBase64("");

  // Assert
  const bool has_value = batch_dleq_proof.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBatchDLEQProofTest, FailToDecodeInvalidBase64) {
  // Arrange

  // Act
  const BatchDLEQProof batch_dleq_proof =
      BatchDLEQProof::DecodeBase64(kInvalidBase64);

  // Assert
  const bool has_value = batch_dleq_proof.has_value();
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBatchDLEQProofTest, EncodeBase64) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::string> encoded_base64 =
      batch_dleq_proof.EncodeBase64();
  ASSERT_TRUE(encoded_base64);

  // Assert
  EXPECT_EQ(kBatchDLEQProofBase64, *encoded_base64);
}

TEST(BatAdsBatchDLEQProofTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof;

  // Act
  const absl::optional<std::string> encoded_base64 =
      batch_dleq_proof.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64);
}

TEST(BatAdsBatchDLEQProofTest, Verify) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_TRUE(batch_dleq_proof.Verify(GetBlindedTokens(), GetSignedTokens(),
                                      GetPublicKey()));
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyWhenUninitialized) {
  // Arrange
  BatchDLEQProof batch_dleq_proof;

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(GetBlindedTokens(), GetSignedTokens(),
                                       GetPublicKey()));
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyWithInvalidBlindedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(GetInvalidBlindedTokens(),
                                       GetSignedTokens(), GetPublicKey()));
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyWithInvalidSignedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(
      GetBlindedTokens(), GetInvalidSignedTokens(), GetPublicKey()));
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyWithMismatchingPublicKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(GetBlindedTokens(), GetSignedTokens(),
                                       GetMismatchingPublicKey()));
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyWithInvalidPublicKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_FALSE(batch_dleq_proof.Verify(GetBlindedTokens(), GetSignedTokens(),
                                       GetInvalidPublicKey()));
}

TEST(BatAdsBatchDLEQProofTest, VerifyAndUnblind) {
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

TEST(BatAdsBatchDLEQProofTest, FailToVerifyAndUnblindWhenUninitialized) {
  // Arrange
  BatchDLEQProof batch_dleq_proof;

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetBlindedTokens(),
                                        GetSignedTokens(), GetPublicKey());

  // Assert
  ASSERT_FALSE(unblinded_tokens);
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithInvalidTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(GetInvalidTokens(), GetBlindedTokens(),
                                        GetSignedTokens(), GetPublicKey());

  // Assert
  EXPECT_FALSE(unblinded_tokens);
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithInvalidBlindedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetInvalidBlindedTokens(),
                                        GetSignedTokens(), GetPublicKey());

  // Assert
  EXPECT_FALSE(unblinded_tokens);
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithInvalidSignedTokens) {
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

TEST(BatAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithMismatchingPublicKey) {
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

TEST(BatAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithInvalidPublicKey) {
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

TEST(BatAdsBatchDLEQProofTest, IsEqual) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST(BatAdsBatchDLEQProofTest, IsEqualWhenUninitialized) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof;

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST(BatAdsBatchDLEQProofTest, IsEmptyBase64Equal) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof("");

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST(BatAdsBatchDLEQProofTest, IsInvalidBase64Equal) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST(BatAdsBatchDLEQProofTest, IsNotEqual) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  const BatchDLEQProof different_batch_dleq_proof(kInvalidBase64);
  EXPECT_NE(different_batch_dleq_proof, batch_dleq_proof);
}

TEST(BatAdsBatchDLEQProofTest, OutputStream) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  std::stringstream ss;
  ss << batch_dleq_proof;

  // Assert
  EXPECT_EQ(kBatchDLEQProofBase64, ss.str());
}

TEST(BatAdsBatchDLEQProofTest, OutputStreamWhenUninitialized) {
  // Arrange
  const BatchDLEQProof batch_dleq_proof;

  // Act
  std::stringstream ss;
  ss << batch_dleq_proof;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace brave_ads::privacy::cbr
