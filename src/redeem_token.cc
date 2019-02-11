/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "redeem_token.h"
#include "logging.h"
#include "ads_serve_helper.h"
#include "security_helper.h"
#include "create_confirmation_request.h"
#include "fetch_payment_token_request.h"

#include "base/logging.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/values.h"

using namespace std::placeholders;
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

void RedeemToken::Redeem(const std::string& creative_instance_id) {
  DCHECK(!creative_instance_id.empty());

  BLOG(INFO) << "Redeem";

  if (unblinded_tokens_->IsEmpty()) {
    BLOG(INFO) << "No unblinded tokens to redeem";
    return;
  }

  auto unblinded_token = unblinded_tokens_->GetToken();
  CreateConfirmation(creative_instance_id, unblinded_token);
}

///////////////////////////////////////////////////////////////////////////////

void RedeemToken::CreateConfirmation(
    const std::string& creative_instance_id,
    const UnblindedToken& unblinded_token) {
  DCHECK(!creative_instance_id.empty());

  BLOG(INFO) << "CreateConfirmation";

  if (!unblinded_tokens_->TokenExists(unblinded_token)) {
    BLOG(ERROR) << "Failed to redeem token " << unblinded_token.encode_base64()
        << " as unblinded token could not be found";
    OnRedeem(FAILED, unblinded_token);
    return;
  }

  BLOG(INFO) << "POST /v1/confirmation/{confirmation_id}/{credential}";
  CreateConfirmationRequest request;

  auto payment_tokens = helper::Security::GenerateTokens(1);
  auto payment_token = payment_tokens.front();

  auto blinded_payment_tokens = helper::Security::BlindTokens(payment_tokens);
  auto blinded_payment_token = blinded_payment_tokens.front();

  auto confirmation_id = base::GenerateGUID();

  auto payload = request.CreateConfirmationRequestDTO(creative_instance_id,
      blinded_payment_token);

  auto credential = request.CreateCredential(unblinded_token, payload);

  BLOG(INFO) << "URL Request:";

  auto url = request.BuildUrl(confirmation_id, credential);
  BLOG(INFO) << "  URL: " << url;

  auto method = request.GetMethod();

  auto body = request.BuildBody(payload);
  BLOG(INFO) << "  Body: " << body;

  auto headers = request.BuildHeaders();
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header;
  }

  auto content_type = request.GetContentType();
  BLOG(INFO) << "  Content_type: " << content_type;

  auto callback = std::bind(&RedeemToken::OnCreateConfirmation,
      this, url, _1, _2, _3, confirmation_id, payment_token,
      blinded_payment_token, unblinded_token);

  confirmations_client_->LoadURL(url, headers, body, content_type,
      method, callback);
}

void RedeemToken::OnCreateConfirmation(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::string& confirmation_id,
    const Token& payment_token,
    const BlindedToken& blinded_payment_token,
    const UnblindedToken& unblinded_token) {
  DCHECK(!confirmation_id.empty());

  BLOG(INFO) << "OnCreateConfirmation";

  BLOG(INFO) << "URL Request Response:";
  BLOG(INFO) << "  URL: " << url;
  BLOG(INFO) << "  Response Status Code: " << response_status_code;
  BLOG(INFO) << "  Response: " << response;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header.first << ": " << header.second;
  }

  if (response_status_code != 201) {
    BLOG(ERROR) << "Failed to create confirmation";
    OnRedeem(FAILED, unblinded_token);
    return;
  }

  // Parse JSON response
  std::unique_ptr<base::DictionaryValue> dictionary =
      base::DictionaryValue::From(base::JSONReader::Read(response));

  if (!dictionary) {
    BLOG(ERROR) << "Failed to parse response: " << response;
    OnRedeem(FAILED, unblinded_token);
    return;
  }

  // Get id
  auto* id_value = dictionary->FindKey("id");
  if (!id_value) {
    BLOG(ERROR) << "Response missing id";
    OnRedeem(FAILED, unblinded_token);
    return;
  }

  auto id = id_value->GetString();

  // Validate id
  if (id != confirmation_id) {
    BLOG(ERROR) << "Response id: " << id << " does not match confirmation id: "
        << confirmation_id;
    OnRedeem(FAILED, unblinded_token);
    return;
  }

  // Fetch payment token
  FetchPaymentToken(confirmation_id, payment_token, blinded_payment_token,
      unblinded_token);
}

void RedeemToken::FetchPaymentToken(
    const std::string& confirmation_id,
    const Token& payment_token,
    const BlindedToken& blinded_payment_token,
    const UnblindedToken& unblinded_token) {
  DCHECK(!confirmation_id.empty());

  BLOG(INFO) << "FetchPaymentToken";

  BLOG(INFO) << "GET /v1/confirmation/{confirmation_id}/paymentToken";
  FetchPaymentTokenRequest request;

  BLOG(INFO) << "URL Request:";

  auto url = request.BuildUrl(confirmation_id);
  BLOG(INFO) << "  URL: " << url;

  auto method = request.GetMethod();

  auto callback = std::bind(&RedeemToken::OnFetchPaymentToken,
      this, url, _1, _2, _3, payment_token, blinded_payment_token,
      unblinded_token);

  confirmations_client_->LoadURL(url, {}, "", "", method, callback);
}

void RedeemToken::OnFetchPaymentToken(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const Token& payment_token,
    const BlindedToken& blinded_payment_token,
    const UnblindedToken& unblinded_token) {
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
    OnRedeem(FAILED, unblinded_token);
    return;
  }

  // Parse JSON response
  std::unique_ptr<base::DictionaryValue> dictionary =
      base::DictionaryValue::From(base::JSONReader::Read(response));

  if (!dictionary) {
    BLOG(ERROR) << "Failed to parse response: " << response;
    OnRedeem(FAILED, unblinded_token);
    return;
  }

  // Get id
  auto* id_value = dictionary->FindKey("id");
  if (!id_value) {
    BLOG(ERROR) << "Response missing id";
    OnRedeem(FAILED, unblinded_token);
    return;
  }

  auto id = id_value->GetString();

  // Get payment token
  auto* payment_token_value = dictionary->FindKey("paymentToken");
  if (!payment_token_value) {
    BLOG(ERROR) << "Response missing paymentToken";
    OnRedeem(FAILED, unblinded_token);
    return;
  }

  // Get payment token dictionary
  base::DictionaryValue* payment_token_dictionary;
  if (!payment_token_value->GetAsDictionary(&payment_token_dictionary)) {
    BLOG(ERROR) << "Response missing paymentToken dictionary";
    OnRedeem(FAILED, unblinded_token);
    return;
  }

  // Get public key
  auto* public_key_value = payment_token_dictionary->FindKey("publicKey");
  if (!public_key_value) {
    BLOG(ERROR) << "Response missing publicKey in paymentToken dictionary";
    OnRedeem(FAILED, unblinded_token);
    return;
  }
  auto public_key_base64 = public_key_value->GetString();
  auto public_key = PublicKey::decode_base64(public_key_base64);

  // Validate public key
  if (!confirmations_->IsValidPublicKeyForCatalogIssuers(public_key_base64)) {
    BLOG(ERROR) << "Response public_key: " << public_key_base64
        << " was not found in the catalog issuers";
    OnRedeem(FAILED, unblinded_token);
    return;
  }

  // Get batch proof
  auto* batch_proof_value = payment_token_dictionary->FindKey("batchProof");
  if (!batch_proof_value) {
    BLOG(ERROR) << "Response missing batchProof in paymentToken dictionary";
    OnRedeem(FAILED, unblinded_token);
    return;
  }

  auto batch_proof_base64 = batch_proof_value->GetString();
  auto batch_proof = BatchDLEQProof::decode_base64(batch_proof_base64);

  // Get signed tokens
  auto* signed_tokens_value = payment_token_dictionary->FindKey("signedTokens");
  if (!signed_tokens_value) {
    BLOG(ERROR) << "Response missing signedTokens in paymentToken dictionary";
    OnRedeem(FAILED, unblinded_token);
    return;
  }

  base::ListValue signed_token_base64_values(signed_tokens_value->GetList());
  if (signed_token_base64_values.GetSize() != 1) {
    BLOG(ERROR) << "Too many signedTokens";
    OnRedeem(FAILED, unblinded_token);
    return;
  }

  std::vector<SignedToken> signed_tokens;
  for (const auto& signed_token_base64_value : signed_token_base64_values) {
    auto signed_token_base64 = signed_token_base64_value.GetString();
    auto signed_token = SignedToken::decode_base64(signed_token_base64);
    signed_tokens.push_back(signed_token);
  }

  // Verify and unblind payment tokens
  auto payment_tokens = {payment_token};
  auto blinded_payment_tokens = {blinded_payment_token};

  auto unblinded_payment_tokens = batch_proof.verify_and_unblind(
     payment_tokens, blinded_payment_tokens, signed_tokens, public_key);

  if (unblinded_payment_tokens.size() == 0) {
    BLOG(ERROR) << "Failed to verify and unblind payment tokens";

    BLOG(ERROR) << "  Batch proof: " << batch_proof_base64;

    BLOG(ERROR) << "  Payment tokens (" << payment_tokens.size() << "):";
    auto payment_token_base64 = payment_token.encode_base64();
    BLOG(ERROR) << "    " << payment_token_base64;

    BLOG(ERROR) << "  Blinded payment tokens (" << blinded_payment_tokens.size()
        << "):";
    auto blinded_payment_token_base64 = blinded_payment_token.encode_base64();
    BLOG(ERROR) << "    " << blinded_payment_token_base64;

    BLOG(ERROR) << "  Signed tokens (" << signed_tokens.size() << "):";
    for (const auto& signed_token : signed_tokens) {
      auto signed_token_base64 = signed_token.encode_base64();
      BLOG(ERROR) << "    " << signed_token_base64;
    }

    BLOG(ERROR) << "  Public key: " << public_key_base64;

    OnRedeem(FAILED, unblinded_token);
    return;
  }

  // Add tokens
  unblinded_payment_tokens_->AddTokens(unblinded_payment_tokens);

  BLOG(INFO) << "Added " << unblinded_payment_tokens.size()
      << " unblinded payment tokens, you now have "
      << unblinded_payment_tokens_->Count() << " unblinded payment tokens";

  OnRedeem(SUCCESS, unblinded_token);
}

void RedeemToken::OnRedeem(
    const Result result,
    const UnblindedToken& unblinded_token) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to redeem token";
  } else {
    BLOG(INFO) << "Successfully redeemed token";
  }

  if (!unblinded_tokens_->RemoveToken(unblinded_token)) {
    BLOG(ERROR) << "Failed to remove unblinded token "
        << unblinded_token.encode_base64()
        << " as unblinded token could not be found";
  } else {
    BLOG(INFO) << "Removed " << unblinded_token.encode_base64()
        << " unblinded token";
  }

  confirmations_->RefillTokensIfNecessary();
}

}  // namespace confirmations
