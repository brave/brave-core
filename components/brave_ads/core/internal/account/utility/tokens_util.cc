/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/tokens_util.h"

#include <string>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/batch_dleq_proof.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

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

std::optional<cbr::SignedTokenList> ParseSignedTokens(
    const base::Value::Dict& dict) {
  const base::Value::List* const list = dict.FindList(kSignedTokensKey);
  if (!list) {
    return std::nullopt;
  }

  cbr::SignedTokenList signed_tokens;

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

std::optional<cbr::UnblindedTokenList> ParseVerifyAndUnblindTokens(
    const base::Value::Dict& dict,
    const cbr::TokenList& tokens,
    const cbr::BlindedTokenList& blinded_tokens,
    const cbr::PublicKey& public_key) {
  if (!public_key.has_value()) {
    BLOG(1, "Invalid public key");
    return std::nullopt;
  }

  const std::string* const batch_dleq_proof_base64 =
      dict.FindString(kBatchDLEQProofKey);
  if (!batch_dleq_proof_base64) {
    BLOG(1, "Failed to parse batch DLEQ proof");
    return std::nullopt;
  }
  cbr::BatchDLEQProof batch_dleq_proof =
      cbr::BatchDLEQProof(*batch_dleq_proof_base64);
  if (!batch_dleq_proof.has_value()) {
    BLOG(1, "Invalid batch DLEQ proof");
    return std::nullopt;
  }

  const std::optional<cbr::SignedTokenList> signed_tokens =
      ParseSignedTokens(dict);
  if (!signed_tokens) {
    BLOG(1, "Failed to parse signed tokens");
    return std::nullopt;
  }

  const std::optional<cbr::UnblindedTokenList> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(tokens, blinded_tokens, *signed_tokens,
                                        public_key);
  if (!unblinded_tokens || unblinded_tokens->empty()) {
    BLOG(1, "Failed to verify and unblind tokens");
    return std::nullopt;
  }

  return *unblinded_tokens;
}

}  // namespace brave_ads
