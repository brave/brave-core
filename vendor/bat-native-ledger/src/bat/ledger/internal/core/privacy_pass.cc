/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/privacy_pass.h"

#include <string>
#include <utility>
#include <vector>

namespace ledger {

namespace {

using challenge_bypass_ristretto::BatchDLEQProof;
using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::PublicKey;
using challenge_bypass_ristretto::SignedToken;
using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::UnblindedToken;

using BlindedTokenBatch = PrivacyPass::BlindedTokenBatch;

}  // namespace

BlindedTokenBatch::BlindedTokenBatch() = default;
BlindedTokenBatch::~BlindedTokenBatch() = default;
BlindedTokenBatch::BlindedTokenBatch(const BlindedTokenBatch& other) = default;
BlindedTokenBatch& BlindedTokenBatch::operator=(
    const BlindedTokenBatch& other) = default;

BlindedTokenBatch PrivacyPass::CreateBlindedTokens(size_t count) {
  DCHECK_GT(count, size_t(0));
  BlindedTokenBatch batch;
  for (size_t i = 0; i < count; i++) {
    auto t = Token::random();
    batch.tokens.push_back(t.encode_base64());
    batch.blinded_tokens.push_back(t.blind().encode_base64());
  }
  return batch;
}

absl::optional<std::vector<std::string>> PrivacyPass::UnblindTokens(
    const std::vector<std::string>& tokens,
    const std::vector<std::string>& blinded_tokens,
    const std::vector<std::string>& signed_tokens,
    const std::string& batch_proof,
    const std::string& public_key) {
  std::vector<Token> token_objects;
  for (auto& token : tokens) {
    token_objects.push_back(Token::decode_base64(token));
    if (ErrorOccurred()) {
      context().LogError(FROM_HERE) << "Error decoding token";
      return {};
    }
  }

  std::vector<BlindedToken> blinded_token_objects;
  for (auto& blinded_token : blinded_tokens) {
    blinded_token_objects.push_back(BlindedToken::decode_base64(blinded_token));
    if (ErrorOccurred()) {
      context().LogError(FROM_HERE) << "Error decoding blinded token";
      return {};
    }
  }

  std::vector<SignedToken> signed_token_objects;
  for (auto& signed_token : signed_tokens) {
    signed_token_objects.push_back(SignedToken::decode_base64(signed_token));
    if (ErrorOccurred()) {
      context().LogError(FROM_HERE) << "Error decoding signed token";
      return {};
    }
  }

  auto batch_proof_object = BatchDLEQProof::decode_base64(batch_proof);
  if (ErrorOccurred()) {
    context().LogError(FROM_HERE) << "Error decoding batch proof";
    return {};
  }

  auto public_key_object = PublicKey::decode_base64(public_key);
  if (ErrorOccurred()) {
    context().LogError(FROM_HERE) << "Error decoding public key";
    return {};
  }

  auto unblinded_token_objects = batch_proof_object.verify_and_unblind(
      token_objects, blinded_token_objects, signed_token_objects,
      public_key_object);

  if (ErrorOccurred()) {
    context().LogError(FROM_HERE) << "Error unblinding tokens";
    return {};
  }

  std::vector<std::string> unblinded_tokens;
  for (auto& unblinded_token_object : unblinded_token_objects) {
    unblinded_tokens.push_back(unblinded_token_object.encode_base64());
  }

  return unblinded_tokens;
}

absl::optional<PrivacyPass::SignResult> PrivacyPass::SignMessage(
    const std::string& unblinded_token,
    const std::string& message) {
  auto unblinded = UnblindedToken::decode_base64(unblinded_token);
  if (ErrorOccurred()) {
    context().LogError(FROM_HERE) << "Error decoding unblinded token";
    return {};
  }

  auto signed_message = unblinded.derive_verification_key().sign(message);
  std::string preimage = unblinded.preimage().encode_base64();
  std::string signature = signed_message.encode_base64();

  if (ErrorOccurred() || preimage.empty() || signature.empty()) {
    context().LogError(FROM_HERE) << "Error signing message";
    return {};
  }

  return SignResult{.preimage = std::move(preimage),
                    .signature = std::move(signature)};
}

bool PrivacyPass::ErrorOccurred() {
  if (challenge_bypass_ristretto::exception_occurred()) {
    context().LogVerbose(FROM_HERE)
        << challenge_bypass_ristretto::get_last_exception().what();
    return true;
  }
  return false;
}

}  // namespace ledger
