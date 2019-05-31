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

#include "base/logging.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave_base/random.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

using challenge_bypass_ristretto::SignedToken;
using challenge_bypass_ristretto::BatchDLEQProof;
using challenge_bypass_ristretto::PublicKey;

namespace confirmations {

RedeemToken::RedeemToken(
    ConfirmationsImpl* confirmations,
    ConfirmationsClient* confirmations_client,
    UnblindedTokens* unblinded_tokens,
    UnblindedTokens* unblinded_payment_tokens) :
    confirmations_(confirmations),
    confirmations_client_(confirmations_client),
    unblinded_tokens_(unblinded_tokens),
    unblinded_payment_tokens_(unblinded_payment_tokens) {
  BLOG(INFO) << "Initializing redeem token";
}

RedeemToken::~RedeemToken() {
  BLOG(INFO) << "Deinitializing redeem token";
}

void RedeemToken::Redeem(
    const std::string& creative_instance_id,
    const ConfirmationType confirmation_type) {
  DCHECK(!creative_instance_id.empty());

  BLOG(INFO) << "Redeem";

  if (unblinded_tokens_->IsEmpty()) {
    BLOG(INFO) << "No unblinded tokens to redeem";
    return;
  }

  auto token_info = unblinded_tokens_->GetToken();
  CreateConfirmation(creative_instance_id, token_info, confirmation_type);
}

void RedeemToken::Redeem(
    const ConfirmationInfo& confirmation_info) {
  CreateConfirmation(confirmation_info);
}

///////////////////////////////////////////////////////////////////////////////

void RedeemToken::CreateConfirmation(
    const ConfirmationInfo& confirmation_info) {
  BLOG(INFO) << "CreateConfirmation";

  BLOG(INFO) << "POST /v1/confirmation/{confirmation_id}/{credential}";
  CreateConfirmationRequest request;

  BLOG(INFO) << "URL Request:";

  auto url = request.BuildUrl(confirmation_info.id,
      confirmation_info.credential);
  BLOG(INFO) << "  URL: " << url;

  auto method = request.GetMethod();

  auto confirmation_request_dto = request.CreateConfirmationRequestDTO(
      confirmation_info.creative_instance_id,
      confirmation_info.blinded_payment_token,
      confirmation_info.type);

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
      this, url, _1, _2, _3, confirmation_info);

  confirmations_client_->LoadURL(url, headers, body, content_type,
      method, callback);
}

void RedeemToken::CreateConfirmation(
    const std::string& creative_instance_id,
    const TokenInfo& token_info,
    const ConfirmationType confirmation_type) {
  DCHECK(!creative_instance_id.empty());
  DCHECK(unblinded_tokens_->TokenExists(token_info));

  ConfirmationInfo confirmation_info;

  confirmation_info.id = base::GenerateGUID();

  confirmation_info.creative_instance_id = creative_instance_id;

  confirmation_info.type = confirmation_type;

  confirmation_info.token_info = token_info;

  auto payment_tokens = helper::Security::GenerateTokens(1);
  confirmation_info.payment_token = payment_tokens.front();

  auto blinded_payment_tokens = helper::Security::BlindTokens(payment_tokens);
  auto blinded_payment_token = blinded_payment_tokens.front();
  confirmation_info.blinded_payment_token = blinded_payment_token;

  CreateConfirmationRequest request;

  auto confirmation_request_dto =
      request.CreateConfirmationRequestDTO(creative_instance_id,
      blinded_payment_token, confirmation_type);

  confirmation_info.credential =
      request.CreateCredential(token_info, confirmation_request_dto);

  CreateConfirmation(confirmation_info);
}

void RedeemToken::OnCreateConfirmation(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const ConfirmationInfo& confirmation_info) {
  DCHECK(!confirmation_info.id.empty());

  BLOG(INFO) << "OnCreateConfirmation";

  BLOG(INFO) << "URL Request Response:";
  BLOG(INFO) << "  URL: " << url;
  BLOG(INFO) << "  Response Status Code: " << response_status_code;
  BLOG(INFO) << "  Response: " << response;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header.first << ": " << header.second;
  }

  if (response_status_code != 201 && response_status_code != 400) {
    BLOG(ERROR) << "Failed to create confirmation";
    OnRedeem(FAILED, confirmation_info);
    return;
  }

  if (response_status_code != 400) {
    // Parse JSON response
    base::Optional<base::Value> dictionary = base::JSONReader::Read(response);
    if (!dictionary || !dictionary->is_dict()) {
      BLOG(ERROR) << "Failed to parse response: " << response;
      OnRedeem(FAILED, confirmation_info);
      return;
    }

    // Get id
    auto* id_value = dictionary->FindKey("id");
    if (!id_value) {
      BLOG(ERROR) << "Response missing id";
      OnRedeem(FAILED, confirmation_info);
      return;
    }

    auto id = id_value->GetString();

    // Validate id
    if (id != confirmation_info.id) {
      BLOG(ERROR) << "Response id: " << id
          << " does not match confirmation id: " << confirmation_info.id;
      OnRedeem(FAILED, confirmation_info);
      return;
    }
  }

  // Fetch payment token
  FetchPaymentToken(confirmation_info);
}

void RedeemToken::FetchPaymentToken(const ConfirmationInfo& confirmation_info) {
  DCHECK(!confirmation_info.id.empty());

  BLOG(INFO) << "FetchPaymentToken";

  BLOG(INFO) << "GET /v1/confirmation/{confirmation_id}/paymentToken";
  FetchPaymentTokenRequest request;

  BLOG(INFO) << "URL Request:";

  auto url = request.BuildUrl(confirmation_info.id);
  BLOG(INFO) << "  URL: " << url;

  auto method = request.GetMethod();

  auto callback = std::bind(&RedeemToken::OnFetchPaymentToken,
      this, url, _1, _2, _3, confirmation_info);

  confirmations_client_->LoadURL(url, {}, "", "", method, callback);
}

void RedeemToken::OnFetchPaymentToken(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const ConfirmationInfo& confirmation_info) {
  BLOG(INFO) << "OnFetchPaymentToken";

  BLOG(INFO) << "URL Request Response:";
  BLOG(INFO) << "  URL: " << url;
  BLOG(INFO) << "  Response Status Code: " << response_status_code;
  BLOG(INFO) << "  Response: " << response;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header.first << ": " << header.second;
  }

  if (response_status_code != 200) {
    BLOG(ERROR) << "Failed to fetch payment token";
    OnRedeem(FAILED, confirmation_info);
    return;
  }

  // Parse JSON response
  base::Optional<base::Value> dictionary = base::JSONReader::Read(response);
  if (!dictionary || !dictionary->is_dict()) {
    BLOG(ERROR) << "Failed to parse response: " << response;
    OnRedeem(FAILED, confirmation_info);
    return;
  }

  // Get id
  auto* id_value = dictionary->FindKey("id");
  if (!id_value) {
    BLOG(ERROR) << "Response missing id";
    OnRedeem(FAILED, confirmation_info);
    return;
  }

  auto id = id_value->GetString();

  // Get payment token
  auto* payment_token_value = dictionary->FindKey("paymentToken");
  if (!payment_token_value) {
    BLOG(ERROR) << "Response missing paymentToken";
    OnRedeem(FAILED, confirmation_info);
    return;
  }

  // Get payment token dictionary
  base::DictionaryValue* payment_token_dictionary;
  if (!payment_token_value->GetAsDictionary(&payment_token_dictionary)) {
    BLOG(ERROR) << "Response missing paymentToken dictionary";
    OnRedeem(FAILED, confirmation_info);
    return;
  }

  // Get public key
  auto* public_key_value = payment_token_dictionary->FindKey("publicKey");
  if (!public_key_value) {
    BLOG(ERROR) << "Response missing publicKey in paymentToken dictionary";
    OnRedeem(FAILED, confirmation_info);
    return;
  }
  auto public_key_base64 = public_key_value->GetString();
  auto public_key = PublicKey::decode_base64(public_key_base64);

  // Validate public key
  if (!confirmations_->IsValidPublicKeyForCatalogIssuers(public_key_base64)) {
    BLOG(ERROR) << "Response public_key: " << public_key_base64
        << " was not found in the catalog issuers";
    OnRedeem(FAILED, confirmation_info);
    return;
  }

  // Get batch proof
  auto* batch_proof_value = payment_token_dictionary->FindKey("batchProof");
  if (!batch_proof_value) {
    BLOG(ERROR) << "Response missing batchProof in paymentToken dictionary";
    OnRedeem(FAILED, confirmation_info);
    return;
  }

  auto batch_proof_base64 = batch_proof_value->GetString();
  auto batch_proof = BatchDLEQProof::decode_base64(batch_proof_base64);

  // Get signed tokens
  auto* signed_tokens_value = payment_token_dictionary->FindKey("signedTokens");
  if (!signed_tokens_value) {
    BLOG(ERROR) << "Response missing signedTokens in paymentToken dictionary";
    OnRedeem(FAILED, confirmation_info);
    return;
  }

  base::ListValue signed_token_base64_values(signed_tokens_value->GetList());
  if (signed_token_base64_values.GetSize() != 1) {
    BLOG(ERROR) << "Too many signedTokens";
    OnRedeem(FAILED, confirmation_info);
    return;
  }

  std::vector<SignedToken> signed_tokens;
  for (const auto& signed_token_base64_value : signed_token_base64_values) {
    auto signed_token_base64 = signed_token_base64_value.GetString();
    auto signed_token = SignedToken::decode_base64(signed_token_base64);
    signed_tokens.push_back(signed_token);
  }

  // Verify and unblind payment token
  auto payment_tokens = {confirmation_info.payment_token};
  auto blinded_payment_tokens = {confirmation_info.blinded_payment_token};

  auto unblinded_payment_tokens = batch_proof.verify_and_unblind(
     payment_tokens, blinded_payment_tokens, signed_tokens, public_key);

  if (unblinded_payment_tokens.size() != 1) {
    BLOG(ERROR) << "Failed to verify and unblind payment tokens";

    BLOG(ERROR) << "  Batch proof: " << batch_proof_base64;

    BLOG(ERROR) << "  Payment tokens (" << payment_tokens.size() << "):";
    auto payment_token_base64 = confirmation_info.payment_token.encode_base64();
    BLOG(ERROR) << "    " << payment_token_base64;

    BLOG(ERROR) << "  Blinded payment tokens (" << blinded_payment_tokens.size()
        << "):";
    auto blinded_payment_token_base64 =
        confirmation_info.blinded_payment_token.encode_base64();
    BLOG(ERROR) << "    " << blinded_payment_token_base64;

    BLOG(ERROR) << "  Signed tokens (" << signed_tokens.size() << "):";
    for (const auto& signed_token : signed_tokens) {
      auto signed_token_base64 = signed_token.encode_base64();
      BLOG(ERROR) << "    " << signed_token_base64;
    }

    BLOG(ERROR) << "  Public key: " << public_key_base64;

    OnRedeem(FAILED, confirmation_info);
    return;
  }

  // Add unblinded payment token
  TokenInfo unblinded_payment_token_info;
  unblinded_payment_token_info.unblinded_token
      = unblinded_payment_tokens.front();
  unblinded_payment_token_info.public_key = public_key_base64;

  if (unblinded_payment_tokens_->TokenExists(unblinded_payment_token_info)) {
    OnRedeem(FAILED, confirmation_info, false);
    return;
  }

  std::vector<TokenInfo> tokens = {unblinded_payment_token_info};
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
      estimated_redemption_value, confirmation_info.type);

  OnRedeem(SUCCESS, confirmation_info, false);
}

void RedeemToken::OnRedeem(
    const Result result,
    const ConfirmationInfo& confirmation_info,
    const bool should_retry) {
  confirmations_->RemoveConfirmationFromQueue(confirmation_info);


  if (result != SUCCESS) {
    BLOG(WARNING) << "Failed to redeem token with "
        << confirmation_info.creative_instance_id
        << " creative instance id for "
        << std::string(confirmation_info.type);

    if (should_retry) {
      BLOG(INFO) << "Retry " << confirmation_info.creative_instance_id
          << " creative instance id for "
          << std::string(confirmation_info.type);

      BLOG(INFO) << "Added " << confirmation_info.creative_instance_id
          << " creative instance id for "
          << std::string(confirmation_info.type)
          << " to the confirmations queue";

      confirmations_->AppendConfirmationToQueue(confirmation_info);
    }
  } else {
    BLOG(INFO) << "Successfully redeemed token with "
        << confirmation_info.creative_instance_id
        << " creative instance id for " << std::string(confirmation_info.type);
  }

  if (unblinded_tokens_->RemoveToken(confirmation_info.token_info)) {
    BLOG(INFO) << "Removed " <<
        confirmation_info.token_info.unblinded_token.encode_base64()
        << " unblinded token";
  }

  ScheduleNextRetryForFailedConfirmations();

  confirmations_->RefillTokensIfNecessary();
}

void RedeemToken::ScheduleNextRetryForFailedConfirmations() const {
  auto start_timer_in = CalculateTimerForNextRetryForFailedConfirmations();
  confirmations_->StartRetryingFailedConfirmations(start_timer_in);
}

uint64_t RedeemToken::CalculateTimerForNextRetryForFailedConfirmations() const {
  auto start_timer_in = kRetryFailedConfirmationsAfterSeconds;
  auto rand_delay = brave_base::random::Geometric(start_timer_in);
  start_timer_in = rand_delay;

  return start_timer_in;
}

}  // namespace confirmations
