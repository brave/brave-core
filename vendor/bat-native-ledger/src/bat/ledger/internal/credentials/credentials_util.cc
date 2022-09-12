/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/credentials/credentials_util.h"

#include "wrapper.hpp"  // NOLINT

namespace ledger {
namespace credential {

using challenge_bypass_ristretto::BatchDLEQProof;
using challenge_bypass_ristretto::PublicKey;
using challenge_bypass_ristretto::SignedToken;
using challenge_bypass_ristretto::UnblindedToken;
using challenge_bypass_ristretto::VerificationKey;
using challenge_bypass_ristretto::VerificationSignature;

std::vector<Token> GenerateCreds(const int count) {
  DCHECK_GT(count, 0);
  std::vector<Token> creds;

  for (auto i = 0; i < count; i++) {
    auto cred = Token::random();
    creds.push_back(cred);
  }

  return creds;
}

std::string GetCredsJSON(const std::vector<Token>& creds) {
  base::Value::List creds_list;
  for (auto & cred : creds) {
    creds_list.Append(cred.encode_base64());
  }

  std::string json;
  base::JSONWriter::Write(creds_list, &json);
  return json;
}

std::vector<BlindedToken> GenerateBlindCreds(const std::vector<Token>& creds) {
  DCHECK_NE(creds.size(), 0UL);

  std::vector<BlindedToken> blinded_creds;
  for (auto cred : creds) {
    auto blinded_cred = cred.blind();

    blinded_creds.push_back(blinded_cred);
  }

  return blinded_creds;
}

std::string GetBlindedCredsJSON(
    const std::vector<BlindedToken>& blinded_creds) {
  base::Value::List blinded_list;
  for (auto & cred : blinded_creds) {
    blinded_list.Append(cred.encode_base64());
  }

  std::string json;
  base::JSONWriter::Write(blinded_list, &json);
  return json;
}

absl::optional<base::Value::List> ParseStringToBaseList(
    const std::string& string_list) {
  absl::optional<base::Value> value = base::JSONReader::Read(string_list);
  if (!value || !value->is_list()) {
    return absl::nullopt;
  }

  return value->GetList().Clone();
}

bool UnBlindCreds(const mojom::CredsBatch& creds_batch,
                  std::vector<std::string>* unblinded_encoded_creds,
                  std::string* error) {
  DCHECK(error && unblinded_encoded_creds);

  auto batch_proof = BatchDLEQProof::decode_base64(creds_batch.batch_proof);

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    *error = std::string(e.what());
    return false;
  }

  auto creds_base64 = ParseStringToBaseList(creds_batch.creds);
  DCHECK(creds_base64.has_value());
  std::vector<Token> creds;
  for (auto& item : creds_base64.value()) {
    const auto cred = Token::decode_base64(item.GetString());
    creds.push_back(cred);
  }

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    *error = std::string(e.what());
    return false;
  }

  auto blinded_creds_base64 = ParseStringToBaseList(creds_batch.blinded_creds);
  DCHECK(blinded_creds_base64.has_value());
  std::vector<BlindedToken> blinded_creds;
  for (auto& item : blinded_creds_base64.value()) {
    const auto blinded_cred = BlindedToken::decode_base64(item.GetString());
    blinded_creds.push_back(blinded_cred);
  }

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    *error = std::string(e.what());
    return false;
  }

  auto signed_creds_base64 = ParseStringToBaseList(creds_batch.signed_creds);
  DCHECK(signed_creds_base64.has_value());
  std::vector<SignedToken> signed_creds;
  for (auto& item : signed_creds_base64.value()) {
    const auto signed_cred = SignedToken::decode_base64(item.GetString());
    signed_creds.push_back(signed_cred);
  }

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    *error = std::string(e.what());
    return false;
  }

  const auto public_key = PublicKey::decode_base64(creds_batch.public_key);

  auto unblinded_cred = batch_proof.verify_and_unblind(
     creds,
     blinded_creds,
     signed_creds,
     public_key);

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    *error = std::string(e.what());
    return false;
  }

  for (auto& cred : unblinded_cred) {
    unblinded_encoded_creds->push_back(cred.encode_base64());
  }

  if (signed_creds.size() != unblinded_encoded_creds->size()) {
    *error = "Unblinded creds size does not match signed creds sent in!";
    return false;
  }

  return true;
}

bool UnBlindCredsMock(const mojom::CredsBatch& creds,
                      std::vector<std::string>* unblinded_encoded_creds) {
  DCHECK(unblinded_encoded_creds);

  auto signed_creds_base64 = ParseStringToBaseList(creds.signed_creds);
  DCHECK(signed_creds_base64.has_value());

  for (auto& item : signed_creds_base64.value()) {
    unblinded_encoded_creds->push_back(item.GetString());
  }

  return true;
}

std::string ConvertRewardTypeToString(const mojom::RewardsType type) {
  switch (type) {
    case mojom::RewardsType::AUTO_CONTRIBUTE: {
      return "auto-contribute";
    }
    case mojom::RewardsType::ONE_TIME_TIP: {
      return "oneoff-tip";
    }
    case mojom::RewardsType::RECURRING_TIP: {
      return "recurring-tip";
    }
    case mojom::RewardsType::PAYMENT: {
      return "payment";
    }
    case mojom::RewardsType::TRANSFER: {
      return "";
    }
  }
}

base::Value::List GenerateCredentials(
    const std::vector<mojom::UnblindedToken>& token_list,
    const std::string& body) {
  base::Value::List credentials;
  for (auto& item : token_list) {
    absl::optional<base::Value::Dict> token;
    if (ledger::is_testing) {
      token = GenerateSuggestionMock(item.token_value, item.public_key, body);
    } else {
      token = GenerateSuggestion(item.token_value, item.public_key, body);
    }

    if (!token) {
      continue;
    }

    credentials.Append(std::move(*token));
  }
  return credentials;
}

absl::optional<base::Value::Dict> GenerateSuggestion(
    const std::string& token_value,
    const std::string& public_key,
    const std::string& body) {
  if (token_value.empty() || public_key.empty() || body.empty()) {
    return absl::nullopt;
  }

  UnblindedToken unblinded = UnblindedToken::decode_base64(token_value);
  VerificationKey verification_key = unblinded.derive_verification_key();
  VerificationSignature signature = verification_key.sign(body);
  const std::string pre_image = unblinded.preimage().encode_base64();

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    return absl::nullopt;
  }

  base::Value::Dict dict;
  dict.Set("t", pre_image);
  dict.Set("publicKey", public_key);
  dict.Set("signature", signature.encode_base64());
  return dict;
}

base::Value::Dict GenerateSuggestionMock(const std::string& token_value,
                                         const std::string& public_key,
                                         const std::string& body) {
  base::Value::Dict dict;
  dict.Set("t", token_value);
  dict.Set("publicKey", public_key);
  dict.Set("signature", token_value);
  return dict;
}

}  // namespace credential
}  // namespace ledger
