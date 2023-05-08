/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/refill_unblinded_tokens/refill_unblinded_tokens.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/notreached.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/refill_unblinded_tokens/get_signed_tokens_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/refill_unblinded_tokens/request_signed_tokens_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_types.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/net/http/http_status_code.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_request_string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/batch_dleq_proof.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_interface.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kRetryAfter = base::Seconds(15);

constexpr size_t kMinimumUnblindedTokens = 20;
constexpr size_t kMaximumUnblindedTokens = 50;

bool ShouldRefillUnblindedTokens() {
  return privacy::UnblindedTokenCount() < kMinimumUnblindedTokens;
}

size_t CalculateAmountOfTokensToRefill() {
  return kMaximumUnblindedTokens - privacy::UnblindedTokenCount();
}

}  // namespace

RefillUnblindedTokens::RefillUnblindedTokens(
    privacy::TokenGeneratorInterface* token_generator)
    : token_generator_(token_generator) {
  CHECK(token_generator_);
}

RefillUnblindedTokens::~RefillUnblindedTokens() {
  delegate_ = nullptr;
}

void RefillUnblindedTokens::MaybeRefill(const WalletInfo& wallet) {
  if (!ConfirmationStateManager::GetInstance().IsInitialized() ||
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
  CHECK(!is_processing_);

  BLOG(1, "Refill unblinded tokens");

  is_processing_ = true;

  nonce_ = {};

  RequestSignedTokens();
}

void RefillUnblindedTokens::RequestSignedTokens() {
  BLOG(1, "RequestSignedTokens");
  BLOG(2, "POST /v3/confirmation/token/{paymentId}");

  const size_t count = CalculateAmountOfTokensToRefill();
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
                     weak_factory_.GetWeakPtr()));
}

void RefillUnblindedTokens::OnRequestSignedTokens(
    const mojom::UrlResponseInfo& url_response) {
  BLOG(1, "OnRequestSignedTokens");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code == net::kHttpUpgradeRequired) {
    BLOG(1, "Failed to request signed tokens as a browser upgrade is required");
    return FailedToRefillUnblindedTokens(/*should_retry*/ false);
  }

  if (url_response.status_code != net::HTTP_CREATED) {
    BLOG(1, "Failed to request signed tokens");
    return FailedToRefillUnblindedTokens(/*should_retry*/ true);
  }

  // Parse JSON response
  const absl::optional<base::Value> root =
      base::JSONReader::Read(url_response.body);
  if (!root || !root->is_dict()) {
    BLOG(3, "Failed to parse response: " << url_response.body);
    return FailedToRefillUnblindedTokens(/*should_retry*/ false);
  }
  const base::Value::Dict& dict = root->GetDict();

  // Get nonce
  const std::string* const nonce = dict.FindString("nonce");
  if (!nonce) {
    BLOG(0, "Response is missing nonce");
    return FailedToRefillUnblindedTokens(/*should_retry*/ false);
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
                     weak_factory_.GetWeakPtr()));
}

// TODO(https://github.com/brave/brave-browser/issues/29824): Reduce cognitive
// complexity.
void RefillUnblindedTokens::OnGetSignedTokens(
    const mojom::UrlResponseInfo& url_response) {
  BLOG(1, "OnGetSignedTokens");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code == net::kHttpUpgradeRequired) {
    BLOG(1, "Failed to get signed tokens as a browser upgrade is required");
    return FailedToRefillUnblindedTokens(/*should_retry*/ false);
  }

  if (url_response.status_code != net::HTTP_OK &&
      url_response.status_code != net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Failed to get signed tokens");
    return FailedToRefillUnblindedTokens(/*should_retry*/ true);
  }

  // Parse JSON response
  const absl::optional<base::Value> root =
      base::JSONReader::Read(url_response.body);
  if (!root || !root->is_dict()) {
    BLOG(3, "Failed to parse response: " << url_response.body);
    return FailedToRefillUnblindedTokens(/*should_retry*/ false);
  }
  const base::Value::Dict& dict = root->GetDict();

  // Captcha required, retrieve captcha id from response
  if (url_response.status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(1, "Captcha required");
    const std::string* const captcha_id = dict.FindString("captcha_id");
    if (!captcha_id || captcha_id->empty()) {
      BLOG(0, "Response is missing captcha_id");
      return FailedToRefillUnblindedTokens(/*should_retry*/ false);
    }

    BLOG(1, "Captcha is required to refill unblinded tokens");

    if (delegate_) {
      delegate_->OnCaptchaRequiredToRefillUnblindedTokens(*captcha_id);
    }

    return;
  }

  // Get public key
  const std::string* const public_key_base64 = dict.FindString("publicKey");
  if (!public_key_base64) {
    BLOG(0, "Response is missing publicKey");
    return FailedToRefillUnblindedTokens(/*should_retry*/ false);
  }
  const privacy::cbr::PublicKey public_key =
      privacy::cbr::PublicKey(*public_key_base64);
  if (!public_key.has_value()) {
    BLOG(0, "Invalid public key");
    NOTREACHED_NORETURN();
  }

  // Validate public key
  if (!PublicKeyExistsForIssuerType(IssuerType::kConfirmations,
                                    *public_key_base64)) {
    BLOG(0, "Response public key "
                << *public_key_base64
                << " does not exist in confirmations issuer public keys");

    return FailedToRefillUnblindedTokens(/*should_retry*/ false);
  }

  // Get batch dleq proof
  const std::string* const batch_dleq_proof_base64 =
      dict.FindString("batchProof");
  if (!batch_dleq_proof_base64) {
    BLOG(0, "Response is missing batchProof");

    return FailedToRefillUnblindedTokens(/*should_retry*/ false);
  }

  privacy::cbr::BatchDLEQProof batch_dleq_proof =
      privacy::cbr::BatchDLEQProof(*batch_dleq_proof_base64);
  if (!batch_dleq_proof.has_value()) {
    BLOG(0, "Invalid batch DLEQ proof");
    NOTREACHED_NORETURN();
  }

  // Get signed tokens
  const auto* const signed_tokens_list = dict.FindList("signedTokens");
  if (!signed_tokens_list) {
    BLOG(0, "Response is missing signedTokens");

    return FailedToRefillUnblindedTokens(/*should_retry*/ false);
  }

  std::vector<privacy::cbr::SignedToken> signed_tokens;
  for (const auto& item : *signed_tokens_list) {
    CHECK(item.is_string());
    const std::string& signed_token_base64 = item.GetString();
    const privacy::cbr::SignedToken signed_token =
        privacy::cbr::SignedToken(signed_token_base64);
    if (!signed_token.has_value()) {
      NOTREACHED_NORETURN();
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

    return FailedToRefillUnblindedTokens(/*should_retry*/ false);
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
      NOTREACHED_NORETURN();
    }

    const absl::optional<std::string> signature =
        crypto::Sign(*unblinded_token_base64, wallet_.secret_key);
    if (!signature) {
      NOTREACHED_NORETURN();
    }
    unblinded_token.signature = *signature;

    CHECK(IsValid(unblinded_token));

    unblinded_tokens.push_back(unblinded_token);
  }

  AddUnblindedTokens(unblinded_tokens);

  BLOG(1, "Added " << unblinded_tokens.size()
                   << " unblinded tokens, you now have "
                   << privacy::UnblindedTokenCount() << " unblinded tokens");

  SuccessfullyRefilledUnblindedTokens();
}

void RefillUnblindedTokens::SuccessfullyRefilledUnblindedTokens() {
  BLOG(1, "Successfully refilled unblinded tokens");

  retry_timer_.Stop();

  blinded_tokens_.clear();
  tokens_.clear();

  is_processing_ = false;

  if (delegate_) {
    delegate_->OnDidRefillUnblindedTokens();
  }
}

void RefillUnblindedTokens::FailedToRefillUnblindedTokens(
    const bool should_retry) {
  BLOG(1, "Failed to refill unblinded tokens");

  if (delegate_) {
    delegate_->OnFailedToRefillUnblindedTokens();
  }

  if (should_retry) {
    return Retry();
  }

  is_processing_ = false;
}

void RefillUnblindedTokens::Retry() {
  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&RefillUnblindedTokens::OnRetry,
                     weak_factory_.GetWeakPtr()));

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

}  // namespace brave_ads
