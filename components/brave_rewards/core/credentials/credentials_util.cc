/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/credentials/credentials_util.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/challenge_bypass_ristretto/batch_dleq_proof.h"
#include "brave/components/challenge_bypass_ristretto/public_key.h"
#include "brave/components/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/challenge_bypass_ristretto/token_preimage.h"
#include "brave/components/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/challenge_bypass_ristretto/verification_key.h"
#include "brave/components/challenge_bypass_ristretto/verification_signature.h"

namespace brave_rewards::internal {
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
    Token cred = Token::Random();
    creds.emplace_back(std::move(cred));
  }

  return creds;
}

std::string GetCredsJSON(const std::vector<Token>& creds) {
  base::Value::List creds_list;
  for (auto& cred : creds) {
    creds_list.Append(cred.EncodeBase64());
  }

  std::string json;
  base::JSONWriter::Write(creds_list, &json);
  return json;
}

std::vector<BlindedToken> GenerateBlindCreds(const std::vector<Token>& creds) {
  DCHECK_NE(creds.size(), 0UL);

  std::vector<BlindedToken> blinded_creds;
  for (auto cred : creds) {
    BlindedToken blinded_cred = cred.Blind();
    blinded_creds.push_back(std::move(blinded_cred));
  }

  return blinded_creds;
}

std::string GetBlindedCredsJSON(
    const std::vector<BlindedToken>& blinded_creds) {
  base::Value::List blinded_list;
  for (auto& cred : blinded_creds) {
    blinded_list.Append(cred.EncodeBase64());
  }

  std::string json;
  base::JSONWriter::Write(blinded_list, &json);
  return json;
}

std::optional<base::Value::List> ParseStringToBaseList(
    const std::string& string_list) {
  std::optional<base::Value> value = base::JSONReader::Read(string_list);
  if (!value || !value->is_list()) {
    return std::nullopt;
  }

  return value->GetList().Clone();
}

base::expected<std::vector<std::string>, std::string> UnBlindCreds(
    const mojom::CredsBatch& creds_batch) {
  std::vector<std::string> unblinded_encoded_creds;

  auto batch_proof = BatchDLEQProof::DecodeBase64(creds_batch.batch_proof);
  if (!batch_proof.has_value()) {
    return base::unexpected(std::move(batch_proof).error());
  }

  auto creds_base64 = ParseStringToBaseList(creds_batch.creds);
  DCHECK(creds_base64.has_value());
  std::vector<Token> creds;
  for (auto& item : creds_base64.value()) {
    if (auto cred = Token::DecodeBase64(item.GetString()); cred.has_value()) {
      creds.push_back(std::move(cred).value());
    } else {
      return base::unexpected(std::move(cred).error());
    }
  }

  auto blinded_creds_base64 = ParseStringToBaseList(creds_batch.blinded_creds);
  DCHECK(blinded_creds_base64.has_value());
  std::vector<BlindedToken> blinded_creds;
  for (auto& item : blinded_creds_base64.value()) {
    if (auto blinded_cred = BlindedToken::DecodeBase64(item.GetString());
        blinded_cred.has_value()) {
      blinded_creds.push_back(std::move(blinded_cred).value());
    } else {
      return base::unexpected(std::move(blinded_cred).error());
    }
  }

  auto signed_creds_base64 = ParseStringToBaseList(creds_batch.signed_creds);
  DCHECK(signed_creds_base64.has_value());
  std::vector<SignedToken> signed_creds;
  for (auto& item : signed_creds_base64.value()) {
    if (auto signed_cred = SignedToken::DecodeBase64(item.GetString());
        signed_cred.has_value()) {
      signed_creds.push_back(std::move(signed_cred).value());
    } else {
      return base::unexpected(std::move(signed_cred).error());
    }
  }

  auto public_key = PublicKey::DecodeBase64(creds_batch.public_key);
  if (!public_key.has_value()) {
    return base::unexpected(std::move(public_key).error());
  }

  auto unblinded_cred = batch_proof->VerifyAndUnblind(
      creds, blinded_creds, signed_creds, *public_key);

  if (!unblinded_cred.has_value()) {
    return base::unexpected(std::move(unblinded_cred).error());
  }

  for (auto& cred : *unblinded_cred) {
    unblinded_encoded_creds.push_back(cred.EncodeBase64());
  }

  if (signed_creds.size() != unblinded_encoded_creds.size()) {
    return base::unexpected(
        "Unblinded creds size does not match signed creds sent in!");
  }

  return unblinded_encoded_creds;
}

std::vector<std::string> UnBlindCredsMock(const mojom::CredsBatch& creds) {
  std::vector<std::string> unblinded_encoded_creds;

  auto signed_creds_base64 = ParseStringToBaseList(creds.signed_creds);
  DCHECK(signed_creds_base64.has_value());

  for (auto& item : signed_creds_base64.value()) {
    unblinded_encoded_creds.push_back(item.GetString());
  }

  return unblinded_encoded_creds;
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
    std::optional<base::Value::Dict> token;
    if (is_testing) {
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

std::optional<base::Value::Dict> GenerateSuggestion(
    const std::string& token_value,
    const std::string& public_key,
    const std::string& body) {
  if (token_value.empty() || public_key.empty() || body.empty()) {
    return std::nullopt;
  }

  auto unblinded = UnblindedToken::DecodeBase64(token_value);
  if (!unblinded.has_value()) {
    return std::nullopt;
  }

  VerificationKey verification_key = unblinded->DeriveVerificationKey();
  auto signature = verification_key.Sign(body);
  if (!signature.has_value()) {
    return std::nullopt;
  }

  std::string pre_image = unblinded->Preimage().EncodeBase64();
  std::string enconded_signature = signature->EncodeBase64();

  base::Value::Dict dict;
  dict.Set("t", std::move(pre_image));
  dict.Set("publicKey", public_key);
  dict.Set("signature", std::move(enconded_signature));
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
}  // namespace brave_rewards::internal
