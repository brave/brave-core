/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/batch_dleq_proof.h"

#include <sstream>

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token_unittest_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key_unittest_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token_unittest_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signing_key_unittest_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token_unittest_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace privacy {
namespace cbr {

namespace {
constexpr char kBatchDLEQProofBase64[] =
    R"~(y0a409PTX6g97xC0Xq8cCuY7ElLPaP+QJ6DaHNfqlQWizBYCSWdaleakKatkyNswfPmkQuhL7awmzQ0ygEUGDw==)~";
}  // namespace

TEST(BatAdsBatchDLEQProofTest, FailToInitialize) {
  // Arrange
  BatchDLEQProof batch_dleq_proof;

  // Act
  const bool has_value = batch_dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBatchDLEQProofTest, FailToInitializeWithEmptyBase64) {
  // Arrange
  BatchDLEQProof batch_dleq_proof("");

  // Act
  const bool has_value = batch_dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBatchDLEQProofTest, FailToInitializeWithInvalidBase64) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kInvalidBase64);

  // Act
  const bool has_value = batch_dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBatchDLEQProofTest, FailToInitializeWithInvalidBlindedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(GetInvalidBlindedTokens(), GetSignedTokens(),
                                  GetSigningKey());

  // Act
  const bool has_value = batch_dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBatchDLEQProofTest, FailToInitializeWithInvalidSignedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(GetBlindedTokens(), GetInvalidSignedTokens(),
                                  GetSigningKey());

  // Act
  const bool has_value = batch_dleq_proof.has_value();

  // Assert
  EXPECT_FALSE(has_value);
}

TEST(BatAdsBatchDLEQProofTest, FailToInitializeWithInvalidSigningKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(GetBlindedTokens(), GetSignedTokens(),
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
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::string> encoded_base64_optional =
      batch_dleq_proof.EncodeBase64();
  ASSERT_TRUE(encoded_base64_optional);

  const std::string& encoded_base64 = encoded_base64_optional.value();

  // Assert
  EXPECT_EQ(kBatchDLEQProofBase64, encoded_base64);
}

TEST(BatAdsBatchDLEQProofTest, FailToEncodeBase64WhenUninitialized) {
  // Arrange
  BatchDLEQProof batch_dleq_proof;

  // Act
  const absl::optional<std::string> encoded_base64_optional =
      batch_dleq_proof.EncodeBase64();

  // Assert
  EXPECT_FALSE(encoded_base64_optional);
}

TEST(BatAdsBatchDLEQProofTest, Verify) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const bool is_valid = batch_dleq_proof.Verify(
      GetBlindedTokens(), GetSignedTokens(), GetPublicKey());

  // Assert
  EXPECT_TRUE(is_valid);
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyWhenUninitialized) {
  // Arrange
  BatchDLEQProof batch_dleq_proof;

  // Act
  const bool is_valid = batch_dleq_proof.Verify(
      GetBlindedTokens(), GetSignedTokens(), GetPublicKey());

  // Assert
  EXPECT_FALSE(is_valid);
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyWithInvalidBlindedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const bool is_valid = batch_dleq_proof.Verify(
      GetInvalidBlindedTokens(), GetSignedTokens(), GetPublicKey());

  // Assert
  EXPECT_FALSE(is_valid);
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyWithInvalidSignedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const bool is_valid = batch_dleq_proof.Verify(
      GetBlindedTokens(), GetInvalidSignedTokens(), GetPublicKey());

  // Assert
  EXPECT_FALSE(is_valid);
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyWithMismatchingPublicKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const bool is_valid = batch_dleq_proof.Verify(
      GetBlindedTokens(), GetSignedTokens(), GetMismatchingPublicKey());

  // Assert
  EXPECT_FALSE(is_valid);
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyWithInvalidPublicKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const bool is_valid = batch_dleq_proof.Verify(
      GetBlindedTokens(), GetSignedTokens(), GetInvalidPublicKey());

  // Assert
  EXPECT_FALSE(is_valid);
}

TEST(BatAdsBatchDLEQProofTest, VerifyAndUnblind) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens_optional =
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetBlindedTokens(),
                                        GetSignedTokens(), GetPublicKey());
  ASSERT_TRUE(unblinded_tokens_optional);
  const std::vector<UnblindedToken>& unblinded_tokens =
      unblinded_tokens_optional.value();

  // Assert
  EXPECT_EQ(GetUnblindedTokens(), unblinded_tokens);
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyAndUnblindWhenUninitialized) {
  // Arrange
  BatchDLEQProof batch_dleq_proof;

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens_optional =
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetBlindedTokens(),
                                        GetSignedTokens(), GetPublicKey());

  // Assert
  ASSERT_FALSE(unblinded_tokens_optional);
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithInvalidTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens_optional =
      batch_dleq_proof.VerifyAndUnblind(GetInvalidTokens(), GetBlindedTokens(),
                                        GetSignedTokens(), GetPublicKey());

  // Assert
  EXPECT_FALSE(unblinded_tokens_optional);
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithInvalidBlindedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens_optional =
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetInvalidBlindedTokens(),
                                        GetSignedTokens(), GetPublicKey());

  // Assert
  EXPECT_FALSE(unblinded_tokens_optional);
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithInvalidSignedTokens) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens_optional =
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetBlindedTokens(),
                                        GetInvalidSignedTokens(),
                                        GetPublicKey());

  // Assert
  EXPECT_FALSE(unblinded_tokens_optional);
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithMismatchingPublicKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens_optional =
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetBlindedTokens(),
                                        GetSignedTokens(),
                                        GetMismatchingPublicKey());

  // Assert
  EXPECT_FALSE(unblinded_tokens_optional);
}

TEST(BatAdsBatchDLEQProofTest, FailToVerifyAndUnblindWithInvalidPublicKey) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens_optional =
      batch_dleq_proof.VerifyAndUnblind(GetTokens(), GetBlindedTokens(),
                                        GetSignedTokens(),
                                        GetInvalidPublicKey());

  // Assert
  EXPECT_FALSE(unblinded_tokens_optional);
}

TEST(BatAdsBatchDLEQProofTest, IsEqual) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST(BatAdsBatchDLEQProofTest, IsEqualWhenUninitialized) {
  // Arrange
  BatchDLEQProof batch_dleq_proof;

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST(BatAdsBatchDLEQProofTest, IsEmptyBase64Equal) {
  // Arrange
  BatchDLEQProof batch_dleq_proof("");

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST(BatAdsBatchDLEQProofTest, IsInvalidBase64Equal) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kInvalidBase64);

  // Act

  // Assert
  EXPECT_EQ(batch_dleq_proof, batch_dleq_proof);
}

TEST(BatAdsBatchDLEQProofTest, IsNotEqual) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act

  // Assert
  BatchDLEQProof different_batch_dleq_proof(kInvalidBase64);
  EXPECT_NE(different_batch_dleq_proof, batch_dleq_proof);
}

TEST(BatAdsBatchDLEQProofTest, OutputStream) {
  // Arrange
  BatchDLEQProof batch_dleq_proof(kBatchDLEQProofBase64);

  // Act
  std::stringstream ss;
  ss << batch_dleq_proof;

  // Assert
  EXPECT_EQ(kBatchDLEQProofBase64, ss.str());
}

TEST(BatAdsBatchDLEQProofTest, OutputStreamWhenUninitialized) {
  // Arrange
  BatchDLEQProof batch_dleq_proof;

  // Act
  std::stringstream ss;
  ss << batch_dleq_proof;

  // Assert
  EXPECT_TRUE(ss.str().empty());
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
