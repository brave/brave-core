/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens.h"

#include <cstddef>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_types.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_interface.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/request_signed_tokens/request_signed_tokens_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/request_signed_tokens/request_signed_tokens_url_request_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/tokens_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/ads_notifier_manager.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/net/http/http_status_code.h"
#include "brave/components/brave_ads/core/internal/common/url/url_request_string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "net/http/http_status_code.h"

namespace brave_ads {

namespace {
constexpr base::TimeDelta kRetryAfter = base::Seconds(15);
}  // namespace

RefillConfirmationTokens::RefillConfirmationTokens(
    TokenGeneratorInterface* const token_generator)
    : token_generator_(token_generator) {
  CHECK(token_generator_);
}

RefillConfirmationTokens::~RefillConfirmationTokens() {
  delegate_ = nullptr;
}

void RefillConfirmationTokens::MaybeRefill(const WalletInfo& wallet) {
  CHECK(wallet.IsValid());

  if (is_refilling_ || timer_.IsRunning()) {
    return;
  }

  if (!HasIssuers()) {
    BLOG(0, "Failed to refill confirmation tokens due to missing issuers");
    return FailedToRefill();
  }

  if (!ShouldRefillConfirmationTokens()) {
    return BLOG(
        1, "No need to refill confirmation tokens as we already have "
               << ConfirmationTokenCount()
               << " confirmation tokens which is above the minimum threshold");
  }

  wallet_ = wallet;

  Refill();
}

///////////////////////////////////////////////////////////////////////////////

void RefillConfirmationTokens::Refill() {
  CHECK(!is_refilling_);

  is_refilling_ = true;

  NotifyWillRefillConfirmationTokens();

  GenerateTokens();

  RequestSignedTokens();
}

void RefillConfirmationTokens::GenerateTokens() {
  const size_t count = CalculateAmountOfConfirmationTokensToRefill();
  tokens_ = token_generator_->Generate(count);
  blinded_tokens_ = cbr::BlindTokens(*tokens_);
}

bool RefillConfirmationTokens::ShouldRequestSignedTokens() const {
  return !nonce_ || nonce_->empty();
}

void RefillConfirmationTokens::RequestSignedTokens() {
  CHECK(tokens_);
  CHECK(blinded_tokens_);

  BLOG(1, "Request signed tokens");

  RequestSignedTokensUrlRequestBuilder url_request_builder(wallet_,
                                                           *blinded_tokens_);
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  UrlRequest(
      std::move(url_request),
      base::BindOnce(&RefillConfirmationTokens::RequestSignedTokensCallback,
                     weak_factory_.GetWeakPtr()));
}

void RefillConfirmationTokens::RequestSignedTokensCallback(
    const mojom::UrlResponseInfo& url_response) {
  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  const auto result = HandleRequestSignedTokensUrlResponse(url_response);
  if (!result.has_value()) {
    const auto& [failure, should_retry] = result.error();

    BLOG(0, failure);

    return should_retry ? FailedToRefillAndRetry() : FailedToRefill();
  }

  GetSignedTokens();
}

base::expected<void, std::tuple<std::string, bool>>
RefillConfirmationTokens::HandleRequestSignedTokensUrlResponse(
    const mojom::UrlResponseInfo& url_response) {
  if (url_response.status_code == net::kHttpUpgradeRequired) {
    AdsNotifierManager::GetInstance().NotifyBrowserUpgradeRequiredToServeAds();

    return base::unexpected(std::make_tuple(
        "Failed to request signed tokens as a browser upgrade is required",
        /*should_retry=*/false));
  }

  if (url_response.status_code != net::HTTP_CREATED) {
    return base::unexpected(std::make_tuple("Failed to request signed tokens",
                                            /*should_retry=*/true));
  }

  const std::optional<base::Value::Dict> dict =
      base::JSONReader::ReadDict(url_response.body);
  if (!dict) {
    return base::unexpected(std::make_tuple(
        base::StrCat({"Failed to parse response: ", url_response.body}),
        /*should_retry=*/false));
  }

  const bool is_eligible = ParseIsEligible(*dict).value_or(true);
  if (!is_eligible) {
    AdsNotifierManager::GetInstance().NotifyIneligibleRewardsWalletToServeAds();
  }

  nonce_ = ParseNonce(*dict);
  if (!nonce_) {
    return base::unexpected(
        std::make_tuple("Failed to parse nonce", /*should_retry=*/false));
  }

  return base::ok();
}

void RefillConfirmationTokens::GetSignedTokens() {
  CHECK(nonce_);

  BLOG(1, "Get signed tokens");

  GetSignedTokensUrlRequestBuilder url_request_builder(wallet_, *nonce_);
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  UrlRequest(std::move(url_request),
             base::BindOnce(&RefillConfirmationTokens::GetSignedTokensCallback,
                            weak_factory_.GetWeakPtr()));
}

void RefillConfirmationTokens::GetSignedTokensCallback(
    const mojom::UrlResponseInfo& url_response) {
  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  const auto result = HandleGetSignedTokensUrlResponse(url_response);
  if (!result.has_value()) {
    const auto& [failure, should_retry] = result.error();

    BLOG(0, failure);

    return should_retry ? FailedToRefillAndRetry() : FailedToRefill();
  }

  SuccessfullyRefilled();
}

base::expected<void, std::tuple<std::string, bool>>
RefillConfirmationTokens::HandleGetSignedTokensUrlResponse(
    const mojom::UrlResponseInfo& url_response) {
  CHECK(tokens_);
  CHECK(blinded_tokens_);

  if (url_response.status_code == net::kHttpUpgradeRequired) {
    AdsNotifierManager::GetInstance().NotifyBrowserUpgradeRequiredToServeAds();

    return base::unexpected(std::make_tuple(
        "Failed to get signed tokens as a browser upgrade is required",
        /*should_retry=*/false));
  }

  if (url_response.status_code != net::HTTP_OK &&
      url_response.status_code != net::HTTP_UNAUTHORIZED) {
    return base::unexpected(std::make_tuple("Failed to get signed tokens",
                                            /*should_retry=*/true));
  }

  const std::optional<base::Value::Dict> dict =
      base::JSONReader::ReadDict(url_response.body);
  if (!dict) {
    return base::unexpected(std::make_tuple(
        base::StrCat({"Failed to parse response: ", url_response.body}),
        /*should_retry=*/false));
  }

  if (url_response.status_code == net::HTTP_UNAUTHORIZED) {
    ParseAndRequireCaptcha(*dict);

    return base::unexpected(std::make_tuple(
        "Captcha is required to refill confirmation tokens", /*should_retry=*/
        false));
  }

  const std::optional<cbr::PublicKey> public_key = ParsePublicKey(*dict);
  if (!public_key.has_value()) {
    return base::unexpected(std::make_tuple("Failed to parse public key",
                                            /*should_retry=*/false));
  }

  if (!TokenIssuerPublicKeyExistsForType(TokenIssuerType::kConfirmations,
                                         *public_key)) {
    return base::unexpected(
        std::make_tuple("Confirmations public key does not exist",
                        /*should_retry=*/true));
  }

  const auto result = ParseVerifyAndUnblindTokens(
      *dict, *tokens_, *blinded_tokens_, *public_key);
  if (!result.has_value()) {
    BLOG(0, result.error());
    return base::unexpected(
        std::make_tuple("Failed to parse, verify and unblind signed tokens",
                        /*should_retry=*/false));
  }
  const auto& unblinded_tokens = result.value();

  BuildAndAddConfirmationTokens(unblinded_tokens, *public_key, wallet_);

  return base::ok();
}

void RefillConfirmationTokens::ParseAndRequireCaptcha(
    const base::Value::Dict& dict) const {
  if (const std::optional<std::string> captcha_id = ParseCaptchaId(dict)) {
    NotifyCaptchaRequiredToRefillConfirmationTokens(*captcha_id);
  }
}

void RefillConfirmationTokens::SuccessfullyRefilled() {
  Reset();

  NotifyDidRefillConfirmationTokens();
}

void RefillConfirmationTokens::FailedToRefillAndRetry() {
  NotifyFailedToRefillConfirmationTokens();

  Retry();
}

void RefillConfirmationTokens::FailedToRefill() {
  Reset();

  NotifyFailedToRefillConfirmationTokens();
}

void RefillConfirmationTokens::Retry() {
  if (timer_.IsRunning()) {
    // The function `WallClockTimer::PowerSuspendObserver::OnResume` restarts
    // the timer to fire at the desired run time after system power is resumed.
    // It's important to note that URL requests might not succeed upon power
    // restoration, triggering a retry. To avoid initiating a second timer, we
    // refrain from starting another one.
    return;
  }

  const base::Time retry_at = timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&RefillConfirmationTokens::RetryCallback,
                     weak_factory_.GetWeakPtr()));

  NotifyWillRetryRefillingConfirmationTokens(retry_at);
}

void RefillConfirmationTokens::RetryCallback() {
  NotifyDidRetryRefillingConfirmationTokens();

  ShouldRequestSignedTokens() ? RequestSignedTokens() : GetSignedTokens();
}

void RefillConfirmationTokens::StopRetrying() {
  timer_.Stop();
}

void RefillConfirmationTokens::Reset() {
  StopRetrying();

  nonce_.reset();

  tokens_.reset();
  blinded_tokens_.reset();

  is_refilling_ = false;
}

void RefillConfirmationTokens::NotifyWillRefillConfirmationTokens() const {
  if (delegate_) {
    delegate_->OnWillRefillConfirmationTokens();
  }
}

void RefillConfirmationTokens::NotifyCaptchaRequiredToRefillConfirmationTokens(
    const std::string& captcha_id) const {
  if (delegate_) {
    delegate_->OnCaptchaRequiredToRefillConfirmationTokens(captcha_id);
  }
}

void RefillConfirmationTokens::NotifyDidRefillConfirmationTokens() const {
  if (delegate_) {
    delegate_->OnDidRefillConfirmationTokens();
  }
}

void RefillConfirmationTokens::NotifyFailedToRefillConfirmationTokens() const {
  if (delegate_) {
    delegate_->OnFailedToRefillConfirmationTokens();
  }
}

void RefillConfirmationTokens::NotifyWillRetryRefillingConfirmationTokens(
    const base::Time retry_at) const {
  if (delegate_) {
    delegate_->OnWillRetryRefillingConfirmationTokens(retry_at);
  }
}

void RefillConfirmationTokens::NotifyDidRetryRefillingConfirmationTokens()
    const {
  if (delegate_) {
    delegate_->OnDidRetryRefillingConfirmationTokens();
  }
}

}  // namespace brave_ads
