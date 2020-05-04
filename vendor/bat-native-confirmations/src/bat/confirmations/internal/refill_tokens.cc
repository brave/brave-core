/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "bat/confirmations/internal/refill_tokens.h"
#include "bat/confirmations/internal/static_values.h"
#include "bat/confirmations/internal/logging.h"
#include "bat/confirmations/internal/ads_serve_helper.h"
#include "bat/confirmations/internal/privacy_utils.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/unblinded_tokens.h"
#include "bat/confirmations/internal/request_signed_tokens_request.h"
#include "bat/confirmations/internal/get_signed_tokens_request.h"
#include "bat/confirmations/internal/time_util.h"

#include "base/json/json_reader.h"
#include "net/http/http_status_code.h"
#include "brave_base/random.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

using challenge_bypass_ristretto::SignedToken;
using challenge_bypass_ristretto::BatchDLEQProof;
using challenge_bypass_ristretto::PublicKey;

namespace confirmations {

RefillTokens::RefillTokens(
    ConfirmationsImpl* confirmations,
    UnblindedTokens* unblinded_tokens) :
    confirmations_(confirmations),
    unblinded_tokens_(unblinded_tokens) {
}

RefillTokens::~RefillTokens() = default;

void RefillTokens::Refill(
    const WalletInfo& wallet_info,
    const std::string& public_key) {
  DCHECK(!public_key.empty());

  if (retry_timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Refill tokens");

  wallet_info_ = wallet_info;
  if (!wallet_info_.IsValid()) {
    BLOG(0, "Failed to refill tokens due to an invalid wallet");
    return;
  }

  public_key_ = public_key;

  nonce_ = "";

  RequestSignedTokens();
}

///////////////////////////////////////////////////////////////////////////////

void RefillTokens::RequestSignedTokens() {
  BLOG(1, "RequestSignedTokens");

  if (!ShouldRefillTokens()) {
    BLOG(1, "No need to refill tokens as we already have "
        << unblinded_tokens_->Count() << " unblinded tokens which is above the"
            << " minimum threshold of " << kMinimumUnblindedTokens);
    return;
  }

  BLOG(2, "POST /v1/confirmation/token/{payment_id}");

  auto refill_amount = CalculateAmountOfTokensToRefill();
  GenerateAndBlindTokens(refill_amount);

  RequestSignedTokensRequest request;
  auto url = request.BuildUrl(wallet_info_);
  auto method = request.GetMethod();
  auto body = request.BuildBody(blinded_tokens_);
  auto headers = request.BuildHeaders(body, wallet_info_);
  auto content_type = request.GetContentType();

  auto callback = std::bind(&RefillTokens::OnRequestSignedTokens,
      this, _1);

  BLOG(5, UrlRequestToString(url, headers, body, content_type, method));
  confirmations_->get_client()->LoadURL(url, headers, body, content_type,
      method, callback);
}

void RefillTokens::OnRequestSignedTokens(const UrlResponse& url_response) {
  BLOG(1, "OnRequestSignedTokens");

  BLOG(6, UrlResponseToString(url_response));

  if (url_response.status_code != net::HTTP_CREATED) {
    BLOG(1, "Failed to request signed tokens");
    OnRefill(FAILED);
    return;
  }

  // Parse JSON response
  base::Optional<base::Value> dictionary =
      base::JSONReader::Read(url_response.body);
  if (!dictionary || !dictionary->is_dict()) {
    BLOG(3, "Failed to parse response: " << url_response.body);
    OnRefill(FAILED, false);
    return;
  }

  // Get nonce
  auto* nonce_value = dictionary->FindKey("nonce");
  if (!nonce_value) {
    BLOG(0, "Response is missing nonce");
    OnRefill(FAILED, false);
    return;
  }

  nonce_ = nonce_value->GetString();

  // Get signed tokens
  GetSignedTokens();
}

void RefillTokens::GetSignedTokens() {
  BLOG(1, "GetSignedTokens");
  BLOG(2, "GET /v1/confirmation/token/{payment_id}?nonce={nonce}");

  GetSignedTokensRequest request;
  auto url = request.BuildUrl(wallet_info_, nonce_);
  auto method = request.GetMethod();

  auto callback = std::bind(&RefillTokens::OnGetSignedTokens, this, _1);

  BLOG(5, UrlRequestToString(url, {}, "", "", method));
  confirmations_->get_client()->LoadURL(url, {}, "", "", method, callback);
}

void RefillTokens::OnGetSignedTokens(
    const UrlResponse& url_response) {
  BLOG(1, "OnGetSignedTokens");

  BLOG(6, UrlResponseToString(url_response));

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(0, "Failed to get signed tokens");
    OnRefill(FAILED);
    return;
  }

  // Parse JSON response
  base::Optional<base::Value> dictionary =
      base::JSONReader::Read(url_response.body);
  if (!dictionary || !dictionary->is_dict()) {
    BLOG(3, "Failed to parse response: " << url_response.body);
    OnRefill(FAILED, false);
    return;
  }

  // Get public key
  auto* public_key_value = dictionary->FindKey("publicKey");
  if (!public_key_value) {
    BLOG(0, "Response is missing publicKey");
    OnRefill(FAILED, false);
    return;
  }

  auto public_key_base64 = public_key_value->GetString();

  // Validate public key
  if (public_key_base64 != public_key_) {
    BLOG(0, "Response public key " << public_key_value->GetString()
        << " does not match catalog issuers public key " << public_key_);
    OnRefill(FAILED, false);
    return;
  }

  // Get batch proof
  auto* batch_proof_value = dictionary->FindKey("batchProof");
  if (!batch_proof_value) {
    BLOG(0, "Response is missing batchProof");
    OnRefill(FAILED, false);
    return;
  }

  auto batch_proof_base64 = batch_proof_value->GetString();
  auto batch_proof = BatchDLEQProof::decode_base64(batch_proof_base64);

  // Get signed tokens
  auto* signed_tokens_value = dictionary->FindKey("signedTokens");
  if (!signed_tokens_value) {
    BLOG(0, "Response is missing signedTokens");
    OnRefill(FAILED, false);
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
    BLOG(1, "Failed to verify and unblind tokens");

    BLOG(1, "  Batch proof: " << batch_proof_base64);

    BLOG(1, "  Tokens (" << tokens_.size() << "):");
    for (const auto& token : tokens_) {
      auto token_base64 = token.encode_base64();
      BLOG(1, "    " << token_base64);
    }

    BLOG(1, "  Blinded tokens (" << blinded_tokens_.size() << "):");
    for (const auto& blinded_token : blinded_tokens_) {
      auto blinded_token_base64 = blinded_token.encode_base64();
      BLOG(1, "    " << blinded_token_base64);
    }

    BLOG(1, "  Signed tokens (" << signed_tokens.size() << "):");
    for (const auto& signed_token : signed_tokens) {
      auto signed_token_base64 = signed_token.encode_base64();
      BLOG(1, "    " << signed_token_base64);
    }

    BLOG(1, "  Public key: " << public_key_);

    OnRefill(FAILED, false);
    return;
  }

  // Add tokens
  TokenList tokens;
  for (const auto& unblinded_token : unblinded_tokens) {
    TokenInfo token_info;
    token_info.unblinded_token = unblinded_token;
    token_info.public_key = public_key_;

    tokens.push_back(token_info);
  }

  unblinded_tokens_->AddTokens(tokens);

  BLOG(1, "Added " << unblinded_tokens.size() << " unblinded tokens, you now "
      "have " << unblinded_tokens_->Count() << " unblinded tokens");

  OnRefill(SUCCESS, false);
}

void RefillTokens::OnRefill(
    const Result result,
    const bool should_retry) {
  if (result != SUCCESS) {
    BLOG(1, "Failed to refill tokens");

    if (should_retry) {
      const base::Time time = retry_timer_.StartWithBackoff(
          kRetryRefillTokensAfterSeconds, base::BindOnce(&RefillTokens::OnRetry,
              base::Unretained(this)));

      BLOG(1, "Retry refilling tokens " << FriendlyDateAndTime(time));
    }

    return;
  }

  retry_timer_.Stop();

  blinded_tokens_.clear();
  tokens_.clear();
  confirmations_->SaveState();

  BLOG(1, "Successfully refilled tokens");
}

void RefillTokens::OnRetry() {
  BLOG(1, "Retry refilling tokens");

  if (nonce_.empty()) {
    RequestSignedTokens();
  } else {
    GetSignedTokens();
  }
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
  tokens_ = privacy::GenerateTokens(count);
  BLOG(1, "Generated " << tokens_.size() << " tokens");

  blinded_tokens_ = privacy::BlindTokens(tokens_);
  BLOG(1, "Blinded " << blinded_tokens_.size() << " tokens");
}

}  // namespace confirmations
