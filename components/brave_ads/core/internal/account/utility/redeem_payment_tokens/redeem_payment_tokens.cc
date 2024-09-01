/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/url_request_builders/redeem_payment_tokens_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/user_data/redeem_payment_tokens_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_request_string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "net/http/http_status_code.h"

namespace brave_ads {

namespace {
constexpr base::TimeDelta kRetryAfter = base::Minutes(1);
}  // namespace

RedeemPaymentTokens::RedeemPaymentTokens() = default;

RedeemPaymentTokens::~RedeemPaymentTokens() {
  delegate_ = nullptr;
}

void RedeemPaymentTokens::MaybeRedeemAfterDelay(const WalletInfo& wallet) {
  CHECK(wallet.IsValid());

  if (is_redeeming_ || timer_.IsRunning()) {
    return;
  }

  wallet_ = wallet;

  RedeemAfterDelay();
}

///////////////////////////////////////////////////////////////////////////////

void RedeemPaymentTokens::RedeemAfterDelay() {
  CHECK(!timer_.IsRunning());

  const base::Time redeem_at = timer_.Start(
      FROM_HERE, CalculateDelayBeforeRedeemingTokens(),
      base::BindOnce(&RedeemPaymentTokens::Redeem, weak_factory_.GetWeakPtr()));
  SetNextTokenRedemptionAt(redeem_at);

  BLOG(1, "Redeem payment tokens " << FriendlyDateAndTime(redeem_at));
}

void RedeemPaymentTokens::Redeem() {
  CHECK(!is_redeeming_);

  BLOG(1, "Redeem payment tokens");

  if (PaymentTokensIsEmpty()) {
    BLOG(1, "No payment tokens to redeem");
    return ScheduleNextRedemption();
  }

  is_redeeming_ = true;

  const PaymentTokenList& payment_tokens = GetAllPaymentTokens();

  RedeemPaymentTokensUrlRequestBuilder url_request_builder(
      wallet_, payment_tokens,
      BuildRedeemPaymentTokensUserData(payment_tokens));
  mojom::UrlRequestInfoPtr mojom_url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(mojom_url_request));
  BLOG(7, UrlRequestHeadersToString(mojom_url_request));

  GetAdsClient()->UrlRequest(
      std::move(mojom_url_request),
      base::BindOnce(&RedeemPaymentTokens::RedeemCallback,
                     weak_factory_.GetWeakPtr(), payment_tokens));
}

void RedeemPaymentTokens::RedeemCallback(
    const PaymentTokenList& payment_tokens,
    const mojom::UrlResponseInfo& mojom_url_response) {
  BLOG(6, UrlResponseToString(mojom_url_response));
  BLOG(7, UrlResponseHeadersToString(mojom_url_response));

  const auto result = HandleUrlResponse(mojom_url_response);
  if (!result.has_value()) {
    const auto& [failure, should_retry] = result.error();

    BLOG(0, failure);

    return FailedToRedeem(should_retry);
  }

  SuccessfullyRedeemed(payment_tokens);
}

// static
base::expected<void, std::tuple<std::string, bool>>
RedeemPaymentTokens::HandleUrlResponse(
    const mojom::UrlResponseInfo& mojom_url_response) {
  if (mojom_url_response.status_code != net::HTTP_OK) {
    return base::unexpected(std::make_tuple("Failed to redeem payment tokens",
                                            /*should_retry=*/true));
  }

  return base::ok();
}

void RedeemPaymentTokens::SuccessfullyRedeemed(
    const PaymentTokenList& payment_tokens) {
  BLOG(1, "Successfully redeemed payment tokens");

  is_redeeming_ = false;

  StopRetrying();

  RemovePaymentTokens(payment_tokens);

  NotifyDidRedeemPaymentTokens(payment_tokens);

  ScheduleNextRedemption();
}

void RedeemPaymentTokens::FailedToRedeem(const bool should_retry) {
  is_redeeming_ = false;

  if (!should_retry) {
    StopRetrying();
  }

  NotifyFailedToRedeemPaymentTokens();

  if (should_retry) {
    Retry();
  }
}

void RedeemPaymentTokens::ScheduleNextRedemption() {
  const base::Time redeem_at = ScheduleNextTokenRedemptionAt();
  SetNextTokenRedemptionAt(redeem_at);

  NotifyDidScheduleNextPaymentTokenRedemption(redeem_at);

  MaybeRedeemAfterDelay(wallet_);
}

void RedeemPaymentTokens::Retry() {
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
      base::BindOnce(&RedeemPaymentTokens::RetryCallback,
                     weak_factory_.GetWeakPtr()));

  BLOG(1, "Retry redeeming payment tokens " << FriendlyDateAndTime(retry_at));

  NotifyWillRetryRedeemingPaymentTokens(retry_at);
}

void RedeemPaymentTokens::RetryCallback() {
  BLOG(1, "Retry redeeming payment tokens");

  NotifyDidRetryRedeemingPaymentTokens();

  Redeem();
}

void RedeemPaymentTokens::StopRetrying() {
  timer_.Stop();
}

void RedeemPaymentTokens::NotifyDidRedeemPaymentTokens(
    const PaymentTokenList& payment_tokens) const {
  if (delegate_) {
    delegate_->OnDidRedeemPaymentTokens(payment_tokens);
  }
}

void RedeemPaymentTokens::NotifyFailedToRedeemPaymentTokens() const {
  if (delegate_) {
    delegate_->OnFailedToRedeemPaymentTokens();
  }
}

void RedeemPaymentTokens::NotifyDidScheduleNextPaymentTokenRedemption(
    const base::Time redeem_at) const {
  if (delegate_) {
    delegate_->OnDidScheduleNextPaymentTokenRedemption(redeem_at);
  }
}

void RedeemPaymentTokens::NotifyWillRetryRedeemingPaymentTokens(
    const base::Time retry_at) const {
  if (delegate_) {
    delegate_->OnWillRetryRedeemingPaymentTokens(retry_at);
  }
}

void RedeemPaymentTokens::NotifyDidRetryRedeemingPaymentTokens() const {
  if (delegate_) {
    delegate_->OnDidRetryRedeemingPaymentTokens();
  }
}

}  // namespace brave_ads
