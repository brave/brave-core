/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/batch_dleq_proof.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signing_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token_util.h"

namespace ads {
namespace privacy {
namespace cbr {

namespace {

absl::optional<challenge_bypass_ristretto::BatchDLEQProof> Create(
    const std::vector<BlindedToken>& blinded_tokens,
    const std::vector<SignedToken>& signed_tokens,
    const SigningKey& signing_key) {
  if (!signing_key.has_value()) {
    return absl::nullopt;
  }

  const challenge_bypass_ristretto::BatchDLEQProof raw_batch_dleq_proof =
      challenge_bypass_ristretto::BatchDLEQProof(
          ToRawBlindedTokens(blinded_tokens), ToRawSignedTokens(signed_tokens),
          signing_key.get());
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return raw_batch_dleq_proof;
}

absl::optional<challenge_bypass_ristretto::BatchDLEQProof> Create(
    const std::string& batch_dleq_proof_base64) {
  if (batch_dleq_proof_base64.empty()) {
    return absl::nullopt;
  }

  const challenge_bypass_ristretto::BatchDLEQProof raw_batch_dleq_proof =
      challenge_bypass_ristretto::BatchDLEQProof::decode_base64(
          batch_dleq_proof_base64);
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return raw_batch_dleq_proof;
}

}  // namespace

BatchDLEQProof::BatchDLEQProof() = default;

BatchDLEQProof::BatchDLEQProof(const std::string& batch_dleq_proof_base64)
    : batch_dleq_proof_(Create(batch_dleq_proof_base64)) {}

BatchDLEQProof::BatchDLEQProof(const std::vector<BlindedToken>& blinded_tokens,
                               const std::vector<SignedToken>& signed_tokens,
                               const SigningKey& signing_key)
    : batch_dleq_proof_(Create(blinded_tokens, signed_tokens, signing_key)) {}

BatchDLEQProof::BatchDLEQProof(const BatchDLEQProof& other) = default;

BatchDLEQProof::~BatchDLEQProof() = default;

bool BatchDLEQProof::operator==(const BatchDLEQProof& rhs) const {
  return EncodeBase64().value_or("") == rhs.EncodeBase64().value_or("");
}

bool BatchDLEQProof::operator!=(const BatchDLEQProof& rhs) const {
  return !(*this == rhs);
}

BatchDLEQProof BatchDLEQProof::DecodeBase64(
    const std::string& batch_dleq_proof_base64) {
  return BatchDLEQProof(batch_dleq_proof_base64);
}

absl::optional<std::string> BatchDLEQProof::EncodeBase64() const {
  if (!has_value()) {
    return absl::nullopt;
  }

  const std::string encoded_base64 = batch_dleq_proof_->encode_base64();
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return encoded_base64;
}

bool BatchDLEQProof::Verify(const std::vector<BlindedToken>& blinded_tokens,
                            const std::vector<SignedToken>& signed_tokens,
                            const PublicKey& public_key) {
  if (!has_value() || !public_key.has_value()) {
    return false;
  }

  const bool is_valid = batch_dleq_proof_->verify(
      ToRawBlindedTokens(blinded_tokens), ToRawSignedTokens(signed_tokens),
      public_key.get());

  return !ExceptionOccurred() && is_valid;
}

absl::optional<std::vector<UnblindedToken>> BatchDLEQProof::VerifyAndUnblind(
    const std::vector<Token>& tokens,
    const std::vector<BlindedToken>& blinded_tokens,
    const std::vector<SignedToken>& signed_tokens,
    const PublicKey& public_key) {
  if (!has_value() || tokens.empty() || !public_key.has_value()) {
    return absl::nullopt;
  }

  const std::vector<challenge_bypass_ristretto::UnblindedToken>
      raw_unblinded_tokens = batch_dleq_proof_->verify_and_unblind(
          ToRawTokens(tokens), ToRawBlindedTokens(blinded_tokens),
          ToRawSignedTokens(signed_tokens), public_key.get());
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  if (tokens.size() != raw_unblinded_tokens.size()) {
    // An exception is not thrown by FFI if there is a public key mismatch, so
    // detect this edge case
    return absl::nullopt;
  }

  return ToUnblindedTokens(raw_unblinded_tokens);
}

std::ostream& operator<<(std::ostream& os,
                         const BatchDLEQProof& batch_dleq_proof) {
  os << batch_dleq_proof.EncodeBase64().value_or("");
  return os;
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
