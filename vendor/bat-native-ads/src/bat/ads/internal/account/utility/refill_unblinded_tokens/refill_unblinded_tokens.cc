/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/refill_unblinded_tokens/refill_unblinded_tokens.h"

#include <utility>

#include "absl/types/optional.h"
#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/notreached.h"
#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/internal/account/issuers/issuer_types.h"
#include "bat/ads/internal/account/issuers/issuers_util.h"
#include "bat/ads/internal/account/utility/refill_unblinded_tokens/get_signed_tokens_url_request_builder.h"
#include "bat/ads/internal/account/utility/refill_unblinded_tokens/request_signed_tokens_url_request_builder.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/crypto/crypto_util.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/common/net/http/http_status_code.h"
#include "bat/ads/internal/common/time/time_formatting_util.h"
#include "bat/ads/internal/common/url/url_request_string_util.h"
#include "bat/ads/internal/common/url/url_response_string_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/batch_dleq_proof.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "bat/ads/internal/privacy/tokens/token_generator_interface.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "net/http/http_status_code.h"

namespace ads {

namespace {

constexpr base::TimeDelta kRetryAfter = base::Seconds(15);

constexpr int kMinimumUnblindedTokens = 20;
constexpr int kMaximumUnblindedTokens = 50;

bool ShouldRefillUnblindedTokens() {
  return privacy::UnblindedTokenCount() < kMinimumUnblindedTokens;
}

int CalculateAmountOfTokensToRefill() {
  return kMaximumUnblindedTokens - privacy::UnblindedTokenCount();
}

}  // namespace

RefillUnblindedTokens::RefillUnblindedTokens(
    privacy::TokenGeneratorInterface* token_generator)
    : token_generator_(token_generator) {
  DCHECK(token_generator_);
}

RefillUnblindedTokens::~RefillUnblindedTokens() {
  delegate_ = nullptr;
}

void RefillUnblindedTokens::MaybeRefill(const WalletInfo& wallet) {
  if (!ConfirmationStateManager::GetInstance()->IsInitialized() ||
      is_processing_ || retry_timer_.IsRunning()) {
    return;
  }

  if (!wallet.IsValid()) {
    BLOG(0, "Failed to refill unblinded tokens due to an invalid wallet");

    if (delegate_) {
      delegate_->OnFailedToRefillUnblindedTokens();
    }

    return;
  }

  if (!HasIssuers()) {
    BLOG(0, "Failed to refill unblinded tokens due to missing issuers");

    if (delegate_) {
      delegate_->OnFailedToRefillUnblindedTokens();
    }

    return;
  }

  if (!ShouldRefillUnblindedTokens()) {
    BLOG(1, "No need to refill unblinded tokens as we already have "
                << privacy::UnblindedTokenCount()
                << " unblinded tokens which is above the minimum threshold of "
                << kMinimumUnblindedTokens);
    return;
  }

  wallet_ = wallet;

  Refill();
}

///////////////////////////////////////////////////////////////////////////////

void RefillUnblindedTokens::Refill() {
  DCHECK(!is_processing_);

  BLOG(1, "Refill unblinded tokens");

  is_processing_ = true;

  nonce_ = {};

  RequestSignedTokens();
}

void RefillUnblindedTokens::RequestSignedTokens() {
  BLOG(1, "RequestSignedTokens");
  BLOG(2, "POST /v3/confirmation/token/{paymentId}");

  const int count = CalculateAmountOfTokensToRefill();
  tokens_ = token_generator_->Generate(count);

  blinded_tokens_ = privacy::cbr::BlindTokens(tokens_);

  RequestSignedTokensUrlRequestBuilder url_request_builder(wallet_,
                                                           blinded_tokens_);
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  AdsClientHelper::GetInstance()->UrlRequest(
      std::move(url_request),
      base::BindOnce(&RefillUnblindedTokens::OnRequestSignedTokens,
                     base::Unretained(this)));
}

void RefillUnblindedTokens::OnRequestSignedTokens(
    const mojom::UrlResponseInfo& url_response) {
  BLOG(1, "OnRequestSignedTokens");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code == net::kHttpUpgradeRequired) {
    BLOG(1, "Failed to request signed tokens as a browser upgrade is required");
    OnFailedToRefillUnblindedTokens(/*should_retry*/ false);
    return;
  }

  if (url_response.status_code != net::HTTP_CREATED) {
    BLOG(1, "Failed to request signed tokens");
    OnFailedToRefillUnblindedTokens(/*should_retry*/ true);
    return;
  }

  // Parse JSON response
  const absl::optional<base::Value> root =
      base::JSONReader::Read(url_response.body);
  if (!root || !root->is_dict()) {
    BLOG(3, "Failed to parse response: " << url_response.body);
    OnFailedToRefillUnblindedTokens(/*should_retry*/ false);
    return;
  }

  // Get nonce
  const std::string* const nonce = root->FindStringKey("nonce");
  if (!nonce) {
    BLOG(0, "Response is missing nonce");
    OnFailedToRefillUnblindedTokens(/*should_retry*/ false);
    return;
  }
  nonce_ = *nonce;

  GetSignedTokens();
}

void RefillUnblindedTokens::GetSignedTokens() {
  BLOG(1, "GetSignedTokens");
  BLOG(2, "GET /v3/confirmation/token/{paymentId}?nonce={nonce}");

  GetSignedTokensUrlRequestBuilder url_request_builder(wallet_, nonce_);
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  AdsClientHelper::GetInstance()->UrlRequest(
      std::move(url_request),
      base::BindOnce(&RefillUnblindedTokens::OnGetSignedTokens,
                     base::Unretained(this)));
}

void RefillUnblindedTokens::OnGetSignedTokens(
    const mojom::UrlResponseInfo& url_response) {
  BLOG(1, "OnGetSignedTokens");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code == net::kHttpUpgradeRequired) {
    BLOG(1, "Failed to get signed tokens as a browser upgrade is required");
    OnFailedToRefillUnblindedTokens(/*should_retry*/ false);
    return;
  }

  if (url_response.status_code != net::HTTP_OK &&
      url_response.status_code != net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Failed to get signed tokens");
    OnFailedToRefillUnblindedTokens(/*should_retry*/ true);
    return;
  }

  // Parse JSON response
  const absl::optional<base::Value> root =
      base::JSONReader::Read(url_response.body);
  if (!root || !root->is_dict()) {
    BLOG(3, "Failed to parse response: " << url_response.body);
    OnFailedToRefillUnblindedTokens(/*should_retry*/ false);
    return;
  }

  // Captcha required, retrieve captcha id from response
  if (url_response.status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(1, "Captcha required");
    const std::string* const captcha_id = root->FindStringKey("captcha_id");
    if (!captcha_id || captcha_id->empty()) {
      BLOG(0, "Response is missing captcha_id");
      OnFailedToRefillUnblindedTokens(/*should_retry*/ false);
      return;
    }

    BLOG(1, "Captcha is required to refill unblinded tokens");

    if (delegate_) {
      delegate_->OnCaptchaRequiredToRefillUnblindedTokens(*captcha_id);
    }

    return;
  }

  // Get public key
  const std::string* const public_key_base64 = root->FindStringKey("publicKey");
  if (!public_key_base64) {
    BLOG(0, "Response is missing publicKey");
    OnFailedToRefillUnblindedTokens(/*should_retry*/ false);
    return;
  }
  const privacy::cbr::PublicKey public_key =
      privacy::cbr::PublicKey(*public_key_base64);
  if (!public_key.has_value()) {
    BLOG(0, "Invalid public key");
    NOTREACHED();
    OnFailedToRefillUnblindedTokens(/*should_retry*/ false);
    return;
  }

  // Validate public key
  if (!PublicKeyExistsForIssuerType(IssuerType::kConfirmations,
                                    *public_key_base64)) {
    BLOG(0, "Response public key "
                << *public_key_base64
                << " does not exist in confirmations issuer public keys");
    OnFailedToRefillUnblindedTokens(/*should_retry*/ false);
    return;
  }

  // Get batch dleq proof
  const std::string* const batch_dleq_proof_base64 =
      root->FindStringKey("batchProof");
  if (!batch_dleq_proof_base64) {
    BLOG(0, "Response is missing batchProof");
    OnFailedToRefillUnblindedTokens(/*should_retry*/ false);
    return;
  }

  privacy::cbr::BatchDLEQProof batch_dleq_proof =
      privacy::cbr::BatchDLEQProof(*batch_dleq_proof_base64);
  if (!batch_dleq_proof.has_value()) {
    BLOG(0, "Invalid batch DLEQ proof");
    NOTREACHED();
    OnFailedToRefillUnblindedTokens(/*should_retry*/ false);
    return;
  }

  // Get signed tokens
  const base::Value* const signed_tokens_list =
      root->FindListKey("signedTokens");
  if (!signed_tokens_list) {
    BLOG(0, "Response is missing signedTokens");
    OnFailedToRefillUnblindedTokens(/*should_retry*/ false);
    return;
  }

  std::vector<privacy::cbr::SignedToken> signed_tokens;
  for (const auto& item : signed_tokens_list->GetList()) {
    DCHECK(item.is_string());
    const std::string& signed_token_base64 = item.GetString();
    const privacy::cbr::SignedToken signed_token =
        privacy::cbr::SignedToken(signed_token_base64);
    if (!signed_token.has_value()) {
      NOTREACHED();
      continue;
    }

    signed_tokens.push_back(signed_token);
  }

  // Verify and unblind tokens
  const absl::optional<std::vector<privacy::cbr::UnblindedToken>>
      batch_dleq_proof_unblinded_tokens = batch_dleq_proof.VerifyAndUnblind(
          tokens_, blinded_tokens_, signed_tokens, public_key);
  if (!batch_dleq_proof_unblinded_tokens) {
    BLOG(1, "Failed to verify and unblind tokens");
    BLOG(1, "  Batch proof: " << *batch_dleq_proof_base64);
    BLOG(1, "  Public key: " << public_key);

    OnFailedToRefillUnblindedTokens(/*should_retry*/ false);
    return;
  }

  // Add unblinded tokens
  privacy::UnblindedTokenList unblinded_tokens;
  for (const auto& batch_dleq_proof_unblinded_token :
       *batch_dleq_proof_unblinded_tokens) {
    privacy::UnblindedTokenInfo unblinded_token;

    unblinded_token.value = batch_dleq_proof_unblinded_token;

    unblinded_token.public_key = public_key;

    const absl::optional<std::string> unblinded_token_base64 =
        unblinded_token.value.EncodeBase64();
    if (!unblinded_token_base64) {
      NOTREACHED();
      continue;
    }

    const absl::optional<std::string> signature =
        crypto::Sign(*unblinded_token_base64, wallet_.secret_key);
    if (!signature) {
      NOTREACHED();
      continue;
    }
    unblinded_token.signature = *signature;

    DCHECK(IsValid(unblinded_token));

    unblinded_tokens.push_back(unblinded_token);
  }

  AddUnblindedTokens(unblinded_tokens);

  BLOG(1, "Added " << unblinded_tokens.size()
                   << " unblinded tokens, you now have "
                   << privacy::UnblindedTokenCount() << " unblinded tokens");

  OnDidRefillUnblindedTokens();
}

void RefillUnblindedTokens::OnDidRefillUnblindedTokens() {
  BLOG(1, "Successfully refilled unblinded tokens");

  retry_timer_.Stop();

  blinded_tokens_.clear();
  tokens_.clear();

  is_processing_ = false;

  if (delegate_) {
    delegate_->OnDidRefillUnblindedTokens();
  }
}

void RefillUnblindedTokens::OnFailedToRefillUnblindedTokens(
    const bool should_retry) {
  BLOG(1, "Failed to refill unblinded tokens");

  if (delegate_) {
    delegate_->OnFailedToRefillUnblindedTokens();
  }

  if (should_retry) {
    Retry();
    return;
  }

  is_processing_ = false;
}

void RefillUnblindedTokens::Retry() {
  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&RefillUnblindedTokens::OnRetry, base::Unretained(this)));

  BLOG(1, "Retry refilling unblinded tokens " << FriendlyDateAndTime(retry_at));

  if (delegate_) {
    delegate_->OnWillRetryRefillingUnblindedTokens(retry_at);
  }
}

void RefillUnblindedTokens::OnRetry() {
  BLOG(1, "Retry refilling unblinded tokens");

  if (delegate_) {
    delegate_->OnDidRetryRefillingUnblindedTokens();
  }

  if (nonce_.empty()) {
    RequestSignedTokens();
  } else {
    GetSignedTokens();
  }
}

}  // namespace ads
