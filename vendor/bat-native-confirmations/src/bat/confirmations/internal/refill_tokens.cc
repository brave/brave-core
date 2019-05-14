/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "bat/confirmations/internal/refill_tokens.h"
#include "bat/confirmations/internal/static_values.h"
#include "bat/confirmations/internal/logging.h"
#include "bat/confirmations/internal/ads_serve_helper.h"
#include "bat/confirmations/internal/security_helper.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/unblinded_tokens.h"
#include "bat/confirmations/internal/request_signed_tokens_request.h"
#include "bat/confirmations/internal/get_signed_tokens_request.h"

#include "base/logging.h"
#include "base/json/json_reader.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

using challenge_bypass_ristretto::SignedToken;
using challenge_bypass_ristretto::BatchDLEQProof;
using challenge_bypass_ristretto::PublicKey;

namespace confirmations {

RefillTokens::RefillTokens(
    ConfirmationsImpl* confirmations,
    ConfirmationsClient* confirmations_client,
    UnblindedTokens* unblinded_tokens) :
    confirmations_(confirmations),
    confirmations_client_(confirmations_client),
    unblinded_tokens_(unblinded_tokens) {
  BLOG(INFO) << "Initializing refill tokens";
}

RefillTokens::~RefillTokens() {
  BLOG(INFO) << "Deinitializing refill tokens";
}

void RefillTokens::Refill(
    const WalletInfo& wallet_info,
    const std::string& public_key) {
  DCHECK(!wallet_info.payment_id.empty());
  DCHECK(!wallet_info.public_key.empty());
  DCHECK(!public_key.empty());

  BLOG(INFO) << "Refill";

  wallet_info_ = WalletInfo(wallet_info);

  public_key_ = public_key;

  RequestSignedTokens();
}

void RefillTokens::RetryGettingSignedTokens() {
  BLOG(INFO) << "Retry getting signed tokens";

  GetSignedTokens();
}

///////////////////////////////////////////////////////////////////////////////

void RefillTokens::RequestSignedTokens() {
  BLOG(INFO) << "RequestSignedTokens";

  if (!ShouldRefillTokens()) {
    BLOG(INFO) << "No need to refill tokens as we already have "
        << unblinded_tokens_->Count() << " unblinded tokens which is above the"
        << " minimum threshold of " << kMinimumUnblindedTokens;
    return;
  }

  BLOG(INFO) << "POST /v1/confirmation/token/{payment_id}";
  RequestSignedTokensRequest request;

  auto refill_amount = CalculateAmountOfTokensToRefill();
  GenerateAndBlindTokens(refill_amount);

  BLOG(INFO) << "URL Request:";

  auto url = request.BuildUrl(wallet_info_);
  BLOG(INFO) << "  URL: " << url;

  auto method = request.GetMethod();

  auto body = request.BuildBody(blinded_tokens_);
  BLOG(INFO) << "  Body: " << body;

  auto headers = request.BuildHeaders(body, wallet_info_);

  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header;
  }

  auto content_type = request.GetContentType();
  BLOG(INFO) << "  Content_type: " << content_type;

  auto callback = std::bind(&RefillTokens::OnRequestSignedTokens,
      this, url, _1, _2, _3);

  confirmations_client_->LoadURL(url, headers, body, content_type, method,
      callback);
}

void RefillTokens::OnRequestSignedTokens(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  BLOG(INFO) << "OnRequestSignedTokens";

  BLOG(INFO) << "URL Request Response:";
  BLOG(INFO) << "  URL: " << url;
  BLOG(INFO) << "  Response Status Code: " << response_status_code;
  BLOG(INFO) << "  Response: " << response;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header.first << ": " << header.second;
  }

  if (response_status_code != 201) {
    BLOG(ERROR) << "Failed to get blinded tokens";
    OnRefill(FAILED);
    return;
  }

  // Parse JSON response
  base::Optional<base::Value> dictionary = base::JSONReader::Read(response);
  if (!dictionary || !dictionary->is_dict()) {
    BLOG(ERROR) << "Failed to parse response: " << response;
    OnRefill(FAILED);
    return;
  }

  // Get nonce
  auto* nonce_value = dictionary->FindKey("nonce");
  if (!nonce_value) {
    BLOG(ERROR) << "Response missing nonce";
    OnRefill(FAILED);
    return;
  }

  nonce_ = nonce_value->GetString();

  // Get signed tokens
  GetSignedTokens();
}

void RefillTokens::GetSignedTokens() {
  BLOG(INFO) << "GetSignedTokens";

  BLOG(INFO) << "GET /v1/confirmation/token/{payment_id}?nonce={nonce}";
  GetSignedTokensRequest request;

  BLOG(INFO) << "URL Request:";

  auto url = request.BuildUrl(wallet_info_, nonce_);
  BLOG(INFO) << "  URL: " << url;

  auto method = request.GetMethod();

  auto callback = std::bind(&RefillTokens::OnGetSignedTokens,
      this, url, _1, _2, _3);

  confirmations_client_->LoadURL(url, {}, "", "", method, callback);
}

void RefillTokens::OnGetSignedTokens(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  BLOG(INFO) << "OnGetSignedTokens";

  BLOG(INFO) << "URL Request Response:";
  BLOG(INFO) << "  URL: " << url;
  BLOG(INFO) << "  Response Status Code: " << response_status_code;
  BLOG(INFO) << "  Response: " << response;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header.first << ": " << header.second;
  }

  if (response_status_code != 200) {
    BLOG(ERROR) << "Failed to get signed tokens";

    if (response_status_code == 202) {  // Tokens are not ready yet
      confirmations_->StartRetryingToGetRefillSignedTokens(
          kRetryGettingRefillSignedTokensAfterSeconds);
    }

    return;
  }

  // Parse JSON response
  base::Optional<base::Value> dictionary = base::JSONReader::Read(response);
  if (!dictionary || !dictionary->is_dict()) {
    BLOG(ERROR) << "Failed to parse response: " << response;
    OnRefill(FAILED);
    return;
  }

  // Get public key
  auto* public_key_value = dictionary->FindKey("publicKey");
  if (!public_key_value) {
    BLOG(ERROR) << "Response missing publicKey";
    OnRefill(FAILED);
    return;
  }

  auto public_key_base64 = public_key_value->GetString();

  // Validate public key
  if (public_key_base64 != public_key_) {
    BLOG(ERROR) << "Response public_key: " << public_key_value->GetString()
        << " does not match catalog issuers public key: " << public_key_;
    OnRefill(FAILED);
    return;
  }

  // Get batch proof
  auto* batch_proof_value = dictionary->FindKey("batchProof");
  if (!batch_proof_value) {
    BLOG(ERROR) << "Response missing batchProof";
    OnRefill(FAILED);
    return;
  }

  auto batch_proof_base64 = batch_proof_value->GetString();
  auto batch_proof = BatchDLEQProof::decode_base64(batch_proof_base64);

  // Get signed tokens
  auto* signed_tokens_value = dictionary->FindKey("signedTokens");
  if (!signed_tokens_value) {
    BLOG(ERROR) << "Response missing signedTokens";
    OnRefill(FAILED);
    return;
  }

  std::vector<SignedToken> signed_tokens;
  base::ListValue signed_token_base64_values(signed_tokens_value->GetList());
  for (const auto& signed_token_base64_value : signed_token_base64_values) {
    auto signed_token_base64 = signed_token_base64_value.GetString();
    auto signed_token = SignedToken::decode_base64(signed_token_base64);
    signed_tokens.push_back(signed_token);
  }

  // Verify and unblind tokens
  auto unblinded_tokens = batch_proof.verify_and_unblind(tokens_,
      blinded_tokens_, signed_tokens, PublicKey::decode_base64(public_key_));

  if (unblinded_tokens.size() == 0) {
    BLOG(ERROR) << "Failed to verify and unblind tokens";

    BLOG(ERROR) << "  Batch proof: " << batch_proof_base64;

    BLOG(ERROR) << "  Tokens (" << tokens_.size() << "):";
    for (const auto& token : tokens_) {
      auto token_base64 = token.encode_base64();
      BLOG(ERROR) << "    " << token_base64;
    }

    BLOG(ERROR) << "  Blinded tokens (" << blinded_tokens_.size() << "):";
    for (const auto& blinded_token : blinded_tokens_) {
      auto blinded_token_base64 = blinded_token.encode_base64();
      BLOG(ERROR) << "    " << blinded_token_base64;
    }

    BLOG(ERROR) << "  Signed tokens (" << signed_tokens.size() << "):";
    for (const auto& signed_token : signed_tokens) {
      auto signed_token_base64 = signed_token.encode_base64();
      BLOG(ERROR) << "    " << signed_token_base64;
    }

    BLOG(ERROR) << "  Public key: " << public_key_;

    OnRefill(FAILED);
    return;
  }

  // Add tokens
  std::vector<TokenInfo> tokens;
  for (const auto& unblinded_token : unblinded_tokens) {
    TokenInfo token_info;
    token_info.unblinded_token = unblinded_token;
    token_info.public_key = public_key_;

    tokens.push_back(token_info);
  }

  unblinded_tokens_->AddTokens(tokens);

  BLOG(INFO) << "Added " << unblinded_tokens.size()
      << " unblinded tokens, you now have " << unblinded_tokens_->Count()
      << " unblinded tokens";

  OnRefill(SUCCESS);
}

void RefillTokens::OnRefill(const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to refill tokens";
  } else {
    confirmations_->SaveState();

    BLOG(INFO) << "Successfully refilled tokens";
  }

  blinded_tokens_.clear();
  tokens_.clear();
}

bool RefillTokens::ShouldRefillTokens() const {
  if (unblinded_tokens_->Count() >= kMinimumUnblindedTokens) {
    return false;
  }

  return true;
}

int RefillTokens::CalculateAmountOfTokensToRefill() const {
  return kMaximumUnblindedTokens - unblinded_tokens_->Count();
}

void RefillTokens::GenerateAndBlindTokens(const int count) {
  tokens_ = helper::Security::GenerateTokens(count);
  BLOG(INFO) << "Generated " << tokens_.size() << " tokens";

  blinded_tokens_ = helper::Security::BlindTokens(tokens_);
  BLOG(INFO) << "Blinded " << blinded_tokens_.size() << " tokens";
}

}  // namespace confirmations
