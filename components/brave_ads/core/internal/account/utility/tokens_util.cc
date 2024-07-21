/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/tokens_util.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/batch_dleq_proof.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"

namespace brave_ads {

namespace {

constexpr char kBatchDLEQProofKey[] = "batchProof";
constexpr char kSignedTokensKey[] = "signedTokens";
constexpr char kPublicKeyKey[] = "publicKey";

}  // namespace

std::optional<cbr::PublicKey> ParsePublicKey(const base::Value::Dict& dict) {
  const std::string* const public_key_base64 = dict.FindString(kPublicKeyKey);
  if (!public_key_base64) {
    return std::nullopt;
  }

  cbr::PublicKey public_key(*public_key_base64);
  if (!public_key.has_value()) {
    return std::nullopt;
  }

  return public_key;
}

std::optional<std::vector<cbr::SignedToken>> ParseSignedTokens(
    const base::Value::Dict& dict) {
  const base::Value::List* const list = dict.FindList(kSignedTokensKey);
  if (!list) {
    return std::nullopt;
  }

  std::vector<cbr::SignedToken> signed_tokens;

  for (const auto& value : *list) {
    const std::string* const signed_token_base64 = value.GetIfString();
    if (!signed_token_base64) {
      return std::nullopt;
    }

    const cbr::SignedToken signed_token =
        cbr::SignedToken(*signed_token_base64);
    if (!signed_token.has_value()) {
      return std::nullopt;
    }

    signed_tokens.push_back(signed_token);
  }

  return signed_tokens;
}

base::expected<std::vector<cbr::UnblindedToken>, std::string>
ParseVerifyAndUnblindTokens(
    const base::Value::Dict& dict,
    const std::vector<cbr::Token>& tokens,
    const std::vector<cbr::BlindedToken>& blinded_tokens,
    const cbr::PublicKey& public_key) {
  if (!public_key.has_value()) {
    return base::unexpected("Invalid public key");
  }

  const std::string* const batch_dleq_proof_base64 =
      dict.FindString(kBatchDLEQProofKey);
  if (!batch_dleq_proof_base64) {
    return base::unexpected("Failed to parse batch DLEQ proof");
  }
  cbr::BatchDLEQProof batch_dleq_proof =
      cbr::BatchDLEQProof(*batch_dleq_proof_base64);
  if (!batch_dleq_proof.has_value()) {
    return base::unexpected("Invalid batch DLEQ proof");
  }

  const std::optional<std::vector<cbr::SignedToken>> signed_tokens =
      ParseSignedTokens(dict);
  if (!signed_tokens) {
    return base::unexpected("Failed to parse signed tokens");
  }

  const std::optional<std::vector<cbr::UnblindedToken>> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(tokens, blinded_tokens, *signed_tokens,
                                        public_key);
  if (!unblinded_tokens || unblinded_tokens->empty()) {
    return base::unexpected("Failed to verify and unblind tokens");
  }

  return *unblinded_tokens;
}

}  // namespace brave_ads
