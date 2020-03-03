/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "bat/confirmations/internal/redeem_token.h"
#include "bat/confirmations/internal/logging.h"
#include "bat/confirmations/internal/ads_serve_helper.h"
#include "bat/confirmations/internal/security_helper.h"
#include "bat/confirmations/internal/static_values.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/unblinded_tokens.h"
#include "bat/confirmations/internal/create_confirmation_request.h"
#include "bat/confirmations/internal/fetch_payment_token_request.h"
#include "bat/confirmations/internal/time.h"
#include "bat/confirmations/internal/token_info.h"
#include "bat/confirmations/internal/confirmation_info.h"
#include "url/gurl.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

#include "base/logging.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "base/base64.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::SignedToken;
using challenge_bypass_ristretto::BatchDLEQProof;
using challenge_bypass_ristretto::VerificationSignature;
using challenge_bypass_ristretto::PublicKey;

namespace confirmations {

RedeemToken::RedeemToken(
    ConfirmationsImpl* confirmations,
    ConfirmationsClient* confirmations_client,
    UnblindedTokens* unblinded_tokens,
    UnblindedTokens* unblinded_payment_tokens)
    : confirmations_(confirmations),
      confirmations_client_(confirmations_client),
      unblinded_tokens_(unblinded_tokens),
      unblinded_payment_tokens_(unblinded_payment_tokens) {
}

RedeemToken::~RedeemToken() = default;

void RedeemToken::Redeem(
    const AdInfo& info,
    const ConfirmationType confirmation_type) {
  BLOG(INFO) << "Redeem";

  if (unblinded_tokens_->IsEmpty()) {
    BLOG(INFO) << "No unblinded tokens to redeem";
    return;
  }

  auto token_info = unblinded_tokens_->GetToken();
  unblinded_tokens_->RemoveToken(token_info);

  CreateConfirmation(info, confirmation_type, token_info);

  confirmations_->RefillTokensIfNecessary();
}

void RedeemToken::Redeem(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const ConfirmationType confirmation_type) {
  // TODO(https://github.com/brave/brave-browser/issues/8479): Deprecate this
  // function which is used by |ConfirmAction| and replace code with |ConfirmAd|

  DCHECK(!creative_instance_id.empty());
  DCHECK(!creative_set_id.empty());

  AdInfo info;
  info.creative_instance_id = creative_instance_id;
  info.creative_set_id = creative_set_id;
  Redeem(info, confirmation_type);
}

void RedeemToken::Redeem(
    const ConfirmationInfo& confirmation) {
  BLOG(INFO) << "Redeem";

  if (!confirmation.created) {
    CreateConfirmation(confirmation);

    confirmations_->RefillTokensIfNecessary();

    return;
  }

  FetchPaymentToken(confirmation);
}

///////////////////////////////////////////////////////////////////////////////

void RedeemToken::CreateConfirmation(
    const ConfirmationInfo& confirmation) {
  BLOG(INFO) << "CreateConfirmation";

  BLOG(INFO) << "POST /v1/confirmation/{confirmation_id}/{credential}";
  CreateConfirmationRequest request;

  BLOG(INFO) << "URL Request:";

  auto url = request.BuildUrl(confirmation.id, confirmation.credential);
  BLOG(INFO) << "  URL: " << url;

  auto method = request.GetMethod();

  auto confirmation_request_dto =
      request.CreateConfirmationRequestDTO(confirmation);

  auto body = request.BuildBody(confirmation_request_dto);
  BLOG(INFO) << "  Body: " << body;

  auto headers = request.BuildHeaders();
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header;
  }

  auto content_type = request.GetContentType();
  BLOG(INFO) << "  Content_type: " << content_type;

  auto callback = std::bind(&RedeemToken::OnCreateConfirmation,
      this, url, _1, _2, _3, confirmation);

  confirmations_client_->LoadURL(url, headers, body, content_type,
      method, callback);
}

void RedeemToken::CreateConfirmation(
    const AdInfo& ad_info,
    const ConfirmationType confirmation_type,
    const TokenInfo& token_info) {
  DCHECK(!ad_info.creative_instance_id.empty());

  ConfirmationInfo confirmation_info;

  confirmation_info.id = base::GenerateGUID();
  confirmation_info.creative_instance_id = ad_info.creative_instance_id;
  const std::string tld_plus_1 = GetDomainAndRegistry(GURL(ad_info.target_url),
      net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);
  confirmation_info.channel_id = tld_plus_1;
  confirmation_info.type = confirmation_type;
  confirmation_info.token_info = token_info;

  auto payment_tokens = helper::Security::GenerateTokens(1);
  confirmation_info.payment_token = payment_tokens.front();

  auto blinded_payment_tokens = helper::Security::BlindTokens(payment_tokens);
  auto blinded_payment_token = blinded_payment_tokens.front();
  confirmation_info.blinded_payment_token = blinded_payment_token;

  CreateConfirmationRequest request;
  auto payload = request.CreateConfirmationRequestDTO(confirmation_info);
  confirmation_info.credential = request.CreateCredential(token_info, payload);
  confirmation_info.timestamp_in_seconds = Time::NowInSeconds();

  CreateConfirmation(confirmation_info);
}

void RedeemToken::OnCreateConfirmation(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const ConfirmationInfo& confirmation) {
  DCHECK(!confirmation.id.empty());

  BLOG(INFO) << "OnCreateConfirmation";

  BLOG(INFO) << "URL Request Response:";
  BLOG(INFO) << "  URL: " << url;
  BLOG(INFO) << "  Response Status Code: " << response_status_code;
  BLOG(INFO) << "  Response: " << response;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header.first << ": " << header.second;
  }

  // FetchPaymentToken and OnFetchPaymentToken will handle all other HTTP
  // response status codes
  if (response_status_code == net::HTTP_BAD_REQUEST) {
    BLOG(WARNING) << "Duplicate confirmation";
    OnRedeem(FAILED, confirmation, false);

    // Duplicate confirmation so redeem a new token
    Redeem(confirmation);
    return;
  }

  ConfirmationInfo new_confirmation = confirmation;
  new_confirmation.created = true;

  FetchPaymentToken(new_confirmation);
}

void RedeemToken::FetchPaymentToken(
    const ConfirmationInfo& confirmation) {
  DCHECK(!confirmation.id.empty());

  BLOG(INFO) << "FetchPaymentToken";

  BLOG(INFO) << "GET /v1/confirmation/{confirmation_id}/paymentToken";
  FetchPaymentTokenRequest request;

  BLOG(INFO) << "URL Request:";

  auto url = request.BuildUrl(confirmation.id);
  BLOG(INFO) << "  URL: " << url;

  auto method = request.GetMethod();

  auto callback = std::bind(&RedeemToken::OnFetchPaymentToken,
      this, url, _1, _2, _3, confirmation);

  confirmations_client_->LoadURL(url, {}, "", "", method, callback);
}

void RedeemToken::OnFetchPaymentToken(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const ConfirmationInfo& confirmation) {
  BLOG(INFO) << "OnFetchPaymentToken";

  BLOG(INFO) << "URL Request Response:";
  BLOG(INFO) << "  URL: " << url;
  BLOG(INFO) << "  Response Status Code: " << response_status_code;
  BLOG(INFO) << "  Response: " << response;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header.first << ": " << header.second;
  }

  if (response_status_code == net::HTTP_NOT_FOUND) {
    BLOG(WARNING) << "Confirmation not found";

    if (!Verify(confirmation)) {
      BLOG(ERROR) << "Failed to verify confirmation";
      OnRedeem(FAILED, confirmation, false);
      return;
    }

    ConfirmationInfo new_confirmation = confirmation;
    new_confirmation.created = false;

    OnRedeem(FAILED, new_confirmation);
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    BLOG(ERROR) << "Failed to fetch payment token";
    OnRedeem(FAILED, confirmation);
    return;
  }

  // Parse JSON response
  base::Optional<base::Value> dictionary = base::JSONReader::Read(response);
  if (!dictionary || !dictionary->is_dict()) {
    BLOG(ERROR) << "Failed to parse response: " << response;
    OnRedeem(FAILED, confirmation);
    return;
  }

  // Get id
  auto* id_value = dictionary->FindKey("id");
  if (!id_value) {
    BLOG(ERROR) << "Response missing id";
    OnRedeem(FAILED, confirmation);
    return;
  }

  auto id = id_value->GetString();

  // Validate id
  if (id != confirmation.id) {
    BLOG(ERROR) << "Response id: " << id
        << " does not match confirmation id: " << confirmation.id;
    OnRedeem(FAILED, confirmation, false);
    return;
  }

  // Get payment token
  auto* payment_token_value = dictionary->FindKey("paymentToken");
  if (!payment_token_value) {
    BLOG(ERROR) << "Response missing paymentToken";
    OnRedeem(FAILED, confirmation);
    return;
  }

  // Get payment token dictionary
  base::DictionaryValue* payment_token_dictionary;
  if (!payment_token_value->GetAsDictionary(&payment_token_dictionary)) {
    BLOG(ERROR) << "Response missing paymentToken dictionary";
    OnRedeem(FAILED, confirmation, false);

    // Token is in a bad state so redeem a new token
    Redeem(confirmation);
    return;
  }

  // Get public key
  auto* public_key_value = payment_token_dictionary->FindKey("publicKey");
  if (!public_key_value) {
    BLOG(ERROR) << "Response missing publicKey in paymentToken dictionary";
    OnRedeem(FAILED, confirmation);
    return;
  }
  auto public_key_base64 = public_key_value->GetString();
  auto public_key = PublicKey::decode_base64(public_key_base64);

  // Validate public key
  if (!confirmations_->IsValidPublicKeyForCatalogIssuers(public_key_base64)) {
    BLOG(ERROR) << "Response public_key: " << public_key_base64
        << " was not found in the catalog issuers";
    OnRedeem(FAILED, confirmation);
    return;
  }

  // Get batch proof
  auto* batch_proof_value = payment_token_dictionary->FindKey("batchProof");
  if (!batch_proof_value) {
    BLOG(ERROR) << "Response missing batchProof in paymentToken dictionary";
    OnRedeem(FAILED, confirmation);
    return;
  }

  auto batch_proof_base64 = batch_proof_value->GetString();
  auto batch_proof = BatchDLEQProof::decode_base64(batch_proof_base64);

  // Get signed tokens
  auto* signed_tokens_value = payment_token_dictionary->FindKey("signedTokens");
  if (!signed_tokens_value) {
    BLOG(ERROR) << "Response missing signedTokens in paymentToken dictionary";
    OnRedeem(FAILED, confirmation);
    return;
  }

  base::ListValue signed_token_base64_values(signed_tokens_value->GetList());
  if (signed_token_base64_values.GetSize() != 1) {
    BLOG(ERROR) << "Too many signedTokens";
    OnRedeem(FAILED, confirmation);
    return;
  }

  std::vector<SignedToken> signed_tokens;
  for (const auto& signed_token_base64_value : signed_token_base64_values) {
    auto signed_token_base64 = signed_token_base64_value.GetString();
    auto signed_token = SignedToken::decode_base64(signed_token_base64);
    signed_tokens.push_back(signed_token);
  }

  // Verify and unblind payment token
  auto payment_tokens = {confirmation.payment_token};
  auto blinded_payment_tokens = {confirmation.blinded_payment_token};

  auto unblinded_payment_tokens = batch_proof.verify_and_unblind(
     payment_tokens, blinded_payment_tokens, signed_tokens, public_key);

  if (unblinded_payment_tokens.size() != 1) {
    BLOG(ERROR) << "Failed to verify and unblind payment tokens";

    BLOG(ERROR) << "  Batch proof: " << batch_proof_base64;

    BLOG(ERROR) << "  Payment tokens (" << payment_tokens.size() << "):";
    auto payment_token_base64 = confirmation.payment_token.encode_base64();
    BLOG(ERROR) << "    " << payment_token_base64;

    BLOG(ERROR) << "  Blinded payment tokens (" << blinded_payment_tokens.size()
        << "):";
    auto blinded_payment_token_base64 =
        confirmation.blinded_payment_token.encode_base64();
    BLOG(ERROR) << "    " << blinded_payment_token_base64;

    BLOG(ERROR) << "  Signed tokens (" << signed_tokens.size() << "):";
    for (const auto& signed_token : signed_tokens) {
      auto signed_token_base64 = signed_token.encode_base64();
      BLOG(ERROR) << "    " << signed_token_base64;
    }

    BLOG(ERROR) << "  Public key: " << public_key_base64;

    OnRedeem(FAILED, confirmation);
    return;
  }

  // Add unblinded payment token
  TokenInfo unblinded_payment_token_info;
  unblinded_payment_token_info.unblinded_token
      = unblinded_payment_tokens.front();
  unblinded_payment_token_info.public_key = public_key_base64;

  if (unblinded_payment_tokens_->TokenExists(unblinded_payment_token_info)) {
    BLOG(ERROR) << "Duplicate unblinded payment token";
    OnRedeem(FAILED, confirmation, false);
    return;
  }

  TokenList tokens = {unblinded_payment_token_info};
  unblinded_payment_tokens_->AddTokens(tokens);

  // Add transaction to history
  double estimated_redemption_value =
      confirmations_->GetEstimatedRedemptionValue(
          unblinded_payment_token_info.public_key);

  BLOG(INFO)
      << "Added 1 unblinded payment token with an estimated redemption value of"
      << " " << estimated_redemption_value << " BAT, you now have "
      << unblinded_payment_tokens_->Count() << " unblinded payment tokens";

  confirmations_->AppendTransactionToHistory(
      estimated_redemption_value, confirmation.type);

  OnRedeem(SUCCESS, confirmation, false);
}

void RedeemToken::OnRedeem(
    const Result result,
    const ConfirmationInfo& confirmation,
    const bool should_retry) {
  if (result != SUCCESS) {
    BLOG(WARNING) << "Failed to redeem " << confirmation.id
        << " confirmation id with " << confirmation.creative_instance_id
        << " creative instance id for " << std::string(confirmation.type);

    if (should_retry) {
      confirmations_->AppendConfirmationToQueue(confirmation);
    }
  } else {
    BLOG(INFO) << "Successfully redeemed " << confirmation.id
        << " confirmation id with " << confirmation.creative_instance_id
        << " creative instance id for " << std::string(confirmation.type);
  }
}

bool RedeemToken::Verify(
    const ConfirmationInfo& info) const {
  std::string credential;
  base::Base64Decode(info.credential, &credential);

  base::Optional<base::Value> value = base::JSONReader::Read(credential);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  auto* signature_value = dictionary->FindKey("signature");
  if (!signature_value) {
    return false;
  }

  auto signature = signature_value->GetString();
  auto verification_signature = VerificationSignature::decode_base64(signature);

  CreateConfirmationRequest request;
  auto payload = request.CreateConfirmationRequestDTO(info);

  auto unblinded_token = info.token_info.unblinded_token;
  auto verification_key = unblinded_token.derive_verification_key();

  return verification_key.verify(verification_signature, payload);
}

}  // namespace confirmations
