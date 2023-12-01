/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/batch_dleq_proof.h"

#include <utility>

#include "base/containers/span.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signing_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"

namespace brave_ads::cbr {

namespace {

std::optional<challenge_bypass_ristretto::BatchDLEQProof> Create(
    const std::vector<BlindedToken>& blinded_tokens,
    const std::vector<SignedToken>& signed_tokens,
    const SigningKey& signing_key) {
  if (!signing_key.has_value()) {
    return std::nullopt;
  }

  return ValueOrLogError(challenge_bypass_ristretto::BatchDLEQProof::Create(
      ToRawBlindedTokens(blinded_tokens), ToRawSignedTokens(signed_tokens),
      signing_key.get()));
}

std::optional<challenge_bypass_ristretto::BatchDLEQProof> Create(
    const std::string& batch_dleq_proof_base64) {
  if (batch_dleq_proof_base64.empty()) {
    return std::nullopt;
  }

  return ValueOrLogError(
      challenge_bypass_ristretto::BatchDLEQProof::DecodeBase64(
          batch_dleq_proof_base64));
}

std::vector<UnblindedToken> ToUnblindedTokens(
    const std::vector<challenge_bypass_ristretto::UnblindedToken>& raw_tokens) {
  std::vector<UnblindedToken> unblinded_tokens;
  unblinded_tokens.reserve(raw_tokens.size());

  for (const auto& raw_token : raw_tokens) {
    const UnblindedToken unblinded_token(raw_token);
    if (!unblinded_token.has_value()) {
      return {};
    }

    unblinded_tokens.push_back(unblinded_token);
  }

  return unblinded_tokens;
}

}  // namespace

BatchDLEQProof::BatchDLEQProof() = default;

BatchDLEQProof::BatchDLEQProof(const std::string& batch_dleq_proof_base64)
    : batch_dleq_proof_(Create(batch_dleq_proof_base64)) {}

BatchDLEQProof::BatchDLEQProof(const std::vector<BlindedToken>& blinded_tokens,
                               const std::vector<SignedToken>& signed_tokens,
                               const SigningKey& signing_key)
    : batch_dleq_proof_(Create(blinded_tokens, signed_tokens, signing_key)) {}

BatchDLEQProof::~BatchDLEQProof() = default;

bool BatchDLEQProof::operator==(const BatchDLEQProof& other) const {
  return EncodeBase64().value_or("") == other.EncodeBase64().value_or("");
}

bool BatchDLEQProof::operator!=(const BatchDLEQProof& other) const {
  return !(*this == other);
}

BatchDLEQProof BatchDLEQProof::DecodeBase64(
    const std::string& batch_dleq_proof_base64) {
  return BatchDLEQProof(batch_dleq_proof_base64);
}

std::optional<std::string> BatchDLEQProof::EncodeBase64() const {
  if (!has_value()) {
    return std::nullopt;
  }

  if (!batch_dleq_proof_) {
    return std::nullopt;
  }

  return batch_dleq_proof_->EncodeBase64();
}

bool BatchDLEQProof::Verify(const std::vector<BlindedToken>& blinded_tokens,
                            const std::vector<SignedToken>& signed_tokens,
                            const PublicKey& public_key) {
  if (!has_value() || !public_key.has_value()) {
    return false;
  }

  if (!batch_dleq_proof_) {
    return false;
  }

  return ValueOrLogError(
             batch_dleq_proof_->Verify(ToRawBlindedTokens(blinded_tokens),
                                       ToRawSignedTokens(signed_tokens),
                                       public_key.get()))
      .value_or(false);
}

std::optional<std::vector<UnblindedToken>> BatchDLEQProof::VerifyAndUnblind(
    const std::vector<Token>& tokens,
    const std::vector<BlindedToken>& blinded_tokens,
    const std::vector<SignedToken>& signed_tokens,
    const PublicKey& public_key) {
  if (!batch_dleq_proof_ || !has_value() || tokens.empty() ||
      !public_key.has_value()) {
    return std::nullopt;
  }

  auto raw_unblinded_tokens =
      ValueOrLogError(batch_dleq_proof_->VerifyAndUnblind(
          ToRawTokens(tokens), ToRawBlindedTokens(blinded_tokens),
          ToRawSignedTokens(signed_tokens), public_key.get()));
  if (!raw_unblinded_tokens || tokens.size() != raw_unblinded_tokens->size()) {
    // An exception is not thrown by FFI if there is a public key mismatch, so
    // detect this edge case
    return std::nullopt;
  }

  return ToUnblindedTokens(std::move(raw_unblinded_tokens).value());
}

std::ostream& operator<<(std::ostream& os,
                         const BatchDLEQProof& batch_dleq_proof) {
  os << batch_dleq_proof.EncodeBase64().value_or("");
  return os;
}

}  // namespace brave_ads::cbr
