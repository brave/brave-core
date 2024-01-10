/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/challenge_bypass_ristretto/batch_dleq_proof.h"

#include <optional>
#include <utility>

#include "base/types/expected.h"
#include "brave/components/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/challenge_bypass_ristretto/public_key.h"
#include "brave/components/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/challenge_bypass_ristretto/signing_key.h"
#include "brave/components/challenge_bypass_ristretto/token.h"
#include "brave/components/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/third_party/rust/challenge_bypass_ristretto_cxx/src/lib.rs.h"

namespace challenge_bypass_ristretto {

namespace {

rust::Box<cbr_cxx::BlindedTokensResult> ConvertToBlindedTokens(
    const std::vector<BlindedToken>& blinded_tokens) {
  std::vector<std::string> encoded_blinded_tokens;
  encoded_blinded_tokens.reserve(blinded_tokens.size());
  for (const auto& blinded_token : blinded_tokens) {
    encoded_blinded_tokens.push_back(blinded_token.EncodeBase64());
  }
  return cbr_cxx::decode_base64_blinded_tokens(encoded_blinded_tokens);
}

rust::Box<cbr_cxx::SignedTokensResult> ConvertToSignedTokens(
    const std::vector<SignedToken>& signed_tokens) {
  std::vector<std::string> encoded_signed_tokens;
  encoded_signed_tokens.reserve(signed_tokens.size());
  for (const auto& signed_token : signed_tokens) {
    encoded_signed_tokens.push_back(signed_token.EncodeBase64());
  }
  return cbr_cxx::decode_base64_signed_tokens(encoded_signed_tokens);
}

rust::Box<cbr_cxx::TokensResult> ConvertToTokens(
    const std::vector<Token>& tokens) {
  std::vector<std::string> encoded_tokens;
  encoded_tokens.reserve(tokens.size());
  for (const auto& token : tokens) {
    encoded_tokens.push_back(token.EncodeBase64());
  }
  return cbr_cxx::decode_base64_tokens(encoded_tokens);
}

std::optional<std::vector<UnblindedToken>> ConvertFromRawUnblindedTokens(
    const rust::Vec<cbr_cxx::UnblindedToken>& raw_unblinded_tokens) {
  std::vector<UnblindedToken> unblinded_tokens;
  unblinded_tokens.reserve(raw_unblinded_tokens.size());

  for (const auto& unblinded_token : raw_unblinded_tokens) {
    auto encoded_unblinded_token = unblinded_token.encode_base64();
    rust::Box<cbr_cxx::UnblindedTokenResult> unblinded_token_result =
        cbr_cxx::decode_base64_unblinded_token(encoded_unblinded_token);
    if (!unblinded_token_result->is_ok()) {
      return std::nullopt;
    }
    unblinded_tokens.emplace_back(std::move(unblinded_token_result));
  }
  return unblinded_tokens;
}

}  // namespace

BatchDLEQProof::BatchDLEQProof(CxxBatchDLEQProofResultBox raw)
    : raw_(base::MakeRefCounted<CxxBatchDLEQProofRefData>(std::move(raw))) {}

BatchDLEQProof::BatchDLEQProof(const BatchDLEQProof& other) = default;

BatchDLEQProof& BatchDLEQProof::operator=(const BatchDLEQProof& other) =
    default;

BatchDLEQProof::BatchDLEQProof(BatchDLEQProof&& other) noexcept = default;

BatchDLEQProof& BatchDLEQProof::operator=(BatchDLEQProof&& other) noexcept =
    default;

BatchDLEQProof::~BatchDLEQProof() = default;

// static
base::expected<BatchDLEQProof, std::string> BatchDLEQProof::Create(
    const std::vector<BlindedToken>& blinded_tokens,
    const std::vector<SignedToken>& signed_tokens,
    const SigningKey& signing_key) {
  if (blinded_tokens.size() != signed_tokens.size()) {
    return base::unexpected(
        "Blinded tokens and signed tokens must have the same length");
  }

  rust::Box<cbr_cxx::BlindedTokensResult> blinded_tokens_result =
      ConvertToBlindedTokens(blinded_tokens);
  if (!blinded_tokens_result->is_ok()) {
    return base::unexpected(
        "Failed to retrieve blinded tokens for batch DLEQ proof");
  }

  rust::Box<cbr_cxx::SignedTokensResult> signed_tokens_result =
      ConvertToSignedTokens(signed_tokens);
  if (!signed_tokens_result->is_ok()) {
    return base::unexpected(
        "Failed to retrieve signed tokens for batch DLEQ proof");
  }

  CxxBatchDLEQProofResultBox batch_dleq_proof_result =
      signing_key.raw().new_batch_dleq_proof(blinded_tokens_result->unwrap(),
                                             signed_tokens_result->unwrap());

  if (!batch_dleq_proof_result->is_ok()) {
    return base::unexpected("Failed to create new batch DLEQ proof");
  }

  return BatchDLEQProof(std::move(batch_dleq_proof_result));
}

base::expected<bool, std::string> BatchDLEQProof::Verify(
    const std::vector<BlindedToken>& blinded_tokens,
    const std::vector<SignedToken>& signed_tokens,
    const PublicKey& public_key) {
  if (blinded_tokens.size() != signed_tokens.size()) {
    return base::unexpected(
        "Blinded tokens and signed tokens must have the same length");
  }

  rust::Box<cbr_cxx::BlindedTokensResult> blinded_tokens_result =
      ConvertToBlindedTokens(blinded_tokens);
  if (!blinded_tokens_result->is_ok()) {
    return base::unexpected(
        "Failed to retrieve blinded tokens for batch DLEQ proof");
  }

  rust::Box<cbr_cxx::SignedTokensResult> signed_tokens_result =
      ConvertToSignedTokens(signed_tokens);
  if (!signed_tokens_result->is_ok()) {
    return base::unexpected(
        "Failed to retrieve signed tokens for batch DLEQ proof");
  }

  const cbr_cxx::Error error =
      raw().verify(blinded_tokens_result->unwrap(),
                   signed_tokens_result->unwrap(), public_key.raw());
  if (!error.is_ok()) {
    return base::unexpected("Failed to verify batch DLEQ proof");
  }
  return true;
}

base::expected<std::vector<UnblindedToken>, std::string>
BatchDLEQProof::VerifyAndUnblind(
    const std::vector<Token>& tokens,
    const std::vector<BlindedToken>& blinded_tokens,
    const std::vector<SignedToken>& signed_tokens,
    const PublicKey& public_key) {
  if (tokens.size() != blinded_tokens.size() ||
      tokens.size() != signed_tokens.size()) {
    return base::unexpected(
        "Tokens, blinded tokens and signed tokens must have the same length");
  }

  rust::Box<cbr_cxx::BlindedTokensResult> blinded_tokens_result =
      ConvertToBlindedTokens(blinded_tokens);
  if (!blinded_tokens_result->is_ok()) {
    return base::unexpected(
        "Failed to retrieve blinded tokens for batch DLEQ proof");
  }

  rust::Box<cbr_cxx::SignedTokensResult> signed_tokens_result =
      ConvertToSignedTokens(signed_tokens);
  if (!signed_tokens_result->is_ok()) {
    return base::unexpected(
        "Failed to retrieve signed tokens for batch DLEQ proof");
  }

  rust::Box<cbr_cxx::TokensResult> tokens_result = ConvertToTokens(tokens);
  if (!tokens_result->is_ok()) {
    return base::unexpected("Failed to retrieve tokens for batch DLEQ proof");
  }

  rust::Box<cbr_cxx::UnblindedTokensResult> unblinded_tokens_result =
      raw().verify_and_unblind(
          tokens_result->unwrap(), blinded_tokens_result->unwrap(),
          signed_tokens_result->unwrap(), public_key.raw());

  if (!unblinded_tokens_result->is_ok()) {
    return base::unexpected("Failed to verify and unblined batch DLEQ proof");
  }

  std::optional<std::vector<UnblindedToken>> unblinded_tokens =
      ConvertFromRawUnblindedTokens(unblinded_tokens_result->unwrap().as_vec());
  if (!unblinded_tokens) {
    return base::unexpected<std::string>("Failed to decode unblinded token");
  }

  return *unblinded_tokens;
}

base::expected<BatchDLEQProof, std::string> BatchDLEQProof::DecodeBase64(
    const std::string& encoded) {
  CxxBatchDLEQProofResultBox raw_proof_result(
      cbr_cxx::decode_base64_batch_dleq_proof(encoded));

  if (!raw_proof_result->is_ok()) {
    return base::unexpected(raw_proof_result->error().msg.data());
  }

  return BatchDLEQProof(std::move(raw_proof_result));
}

std::string BatchDLEQProof::EncodeBase64() const {
  return static_cast<std::string>(raw().encode_base64());
}

bool BatchDLEQProof::operator==(const BatchDLEQProof& rhs) const {
  return EncodeBase64() == rhs.EncodeBase64();
}

}  // namespace challenge_bypass_ristretto
