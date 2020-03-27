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

using challenge_bypass_ristretto::BatchDLEQProof;
using challenge_bypass_ristretto::PublicKey;
using challenge_bypass_ristretto::SignedToken;
using challenge_bypass_ristretto::UnblindedToken;
using challenge_bypass_ristretto::VerificationKey;
using challenge_bypass_ristretto::VerificationSignature;

namespace braveledger_credentials {

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
  base::Value creds_list(base::Value::Type::LIST);
  for (auto & cred : creds) {
    auto cred_base64 = cred.encode_base64();
    auto cred_value = base::Value(cred_base64);
    creds_list.Append(std::move(cred_value));
  }
  std::string json;
  base::JSONWriter::Write(creds_list, &json);

  return json;
}

std::vector<BlindedToken> GenerateBlindCreds(const std::vector<Token>& creds) {
  DCHECK_NE(creds.size(), 0UL);

  std::vector<BlindedToken> blinded_creds;
  for (unsigned int i = 0; i < creds.size(); i++) {
    auto cred = creds.at(i);
    auto blinded_cred = cred.blind();

    blinded_creds.push_back(blinded_cred);
  }

  return blinded_creds;
}

std::string GetBlindedCredsJSON(
    const std::vector<BlindedToken>& blinded_creds) {
  base::Value blinded_list(base::Value::Type::LIST);
  for (auto & cred : blinded_creds) {
    auto cred_base64 = cred.encode_base64();
    auto cred_value = base::Value(cred_base64);
    blinded_list.Append(std::move(cred_value));
  }
  std::string json;
  base::JSONWriter::Write(blinded_list, &json);

  return json;
}

std::unique_ptr<base::ListValue> ParseStringToBaseList(
    const std::string& string_list) {
  base::Optional<base::Value> value = base::JSONReader::Read(string_list);
  if (!value || !value->is_list()) {
    return std::make_unique<base::ListValue>();
  }

  return std::make_unique<base::ListValue>(value->GetList());
}

bool UnBlindCreds(
    const ledger::CredsBatch& creds_batch,
    std::vector<std::string>* unblinded_encoded_creds,
    std::string* error) {
  DCHECK(error);
  if (!unblinded_encoded_creds) {
    return false;
  }

  auto batch_proof = BatchDLEQProof::decode_base64(creds_batch.batch_proof);

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    *error = std::string(e.what());
    return false;
  }

  auto creds_base64 = ParseStringToBaseList(creds_batch.creds);
  std::vector<Token> creds;
  for (auto& item : *creds_base64) {
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
  std::vector<BlindedToken> blinded_creds;
  for (auto& item : *blinded_creds_base64) {
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
  std::vector<SignedToken> signed_creds;
  for (auto& item : *signed_creds_base64) {
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

bool UnBlindCredsMock(
    const ledger::CredsBatch& creds,
    std::vector<std::string>* unblinded_encoded_creds) {
  if (!unblinded_encoded_creds) {
    return false;
  }

  auto signed_creds_base64 = ParseStringToBaseList(creds.signed_creds);

  for (auto& item : *signed_creds_base64) {
    unblinded_encoded_creds->push_back(item.GetString());
  }

  return true;
}

std::string ConvertRewardTypeToString(const ledger::RewardsType type) {
  switch (type) {
    case ledger::RewardsType::AUTO_CONTRIBUTE: {
      return "auto-contribute";
    }
    case ledger::RewardsType::ONE_TIME_TIP: {
      return "oneoff-tip";
    }
    case ledger::RewardsType::RECURRING_TIP: {
      return "recurring-tip";
    }
    case ledger::RewardsType::TRANSFER: {
      return "";
    }
  }
}

void GenerateCredentials(
    const std::vector<ledger::UnblindedToken>& token_list,
    const std::string& body,
    base::Value* credentials) {
  DCHECK(credentials);

  for (auto& item : token_list) {
    base::Value token(base::Value::Type::DICTIONARY);
    bool success;
    if (ledger::is_testing) {
      success = GenerateSuggestionMock(
          item.token_value,
          item.public_key,
          body,
          &token);
    } else {
      success = GenerateSuggestion(
          item.token_value,
          item.public_key,
          body,
          &token);
    }

    if (!success) {
      continue;
    }

    credentials->Append(std::move(token));
  }
}

bool GenerateSuggestion(
    const std::string& token_value,
    const std::string& public_key,
    const std::string& body,
    base::Value* result) {
  if (token_value.empty() || public_key.empty() || body.empty()) {
    return false;
  }

  UnblindedToken unblinded = UnblindedToken::decode_base64(token_value);
  VerificationKey verification_key = unblinded.derive_verification_key();
  VerificationSignature signature = verification_key.sign(body);
  const std::string pre_image = unblinded.preimage().encode_base64();

  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    return false;
  }

  result->SetStringKey("t", pre_image);
  result->SetStringKey("publicKey", public_key);
  result->SetStringKey("signature", signature.encode_base64());
  return true;
}

bool GenerateSuggestionMock(
    const std::string& token_value,
    const std::string& public_key,
    const std::string& body,
    base::Value* result) {
  result->SetStringKey("t", token_value);
  result->SetStringKey("publicKey", public_key);
  result->SetStringKey("signature", token_value);
  return true;
}

std::string GenerateRedeemTokensPayload(const CredentialsRedeem& redeem) {
  base::Value suggestion(base::Value::Type::DICTIONARY);
  suggestion.SetStringKey(
      "type",
      ConvertRewardTypeToString(redeem.type));
  if (!redeem.order_id.empty()) {
    suggestion.SetStringKey("orderId", redeem.order_id);
  }
  suggestion.SetStringKey("channel", redeem.publisher_key);

  std::string suggestion_json;
  base::JSONWriter::Write(suggestion, &suggestion_json);
  std::string suggestion_encoded;
  base::Base64Encode(suggestion_json, &suggestion_encoded);

  base::Value credentials(base::Value::Type::LIST);
  GenerateCredentials(redeem.token_list, suggestion_encoded, &credentials);

  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetStringKey("suggestion", suggestion_encoded);
  payload.SetKey("credentials", std::move(credentials));

  std::string json;
  base::JSONWriter::Write(payload, &json);
  return json;
}

std::string GenerateTransferTokensPayload(
    const CredentialsRedeem& redeem,
    const std::string& payment_id) {
  base::Value credentials(base::Value::Type::LIST);
  GenerateCredentials(redeem.token_list, payment_id, &credentials);

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("paymentId", payment_id);
  body.SetKey("credentials", std::move(credentials));

  std::string json;
  base::JSONWriter::Write(body, &json);
  return json;
}

}  // namespace braveledger_credentials
