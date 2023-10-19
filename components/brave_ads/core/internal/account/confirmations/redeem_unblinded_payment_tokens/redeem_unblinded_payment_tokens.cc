/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"

#include <cstdint>
#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_util.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_request_string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"
#include "net/http/http_status_code.h"

namespace brave_ads {

namespace {
constexpr base::TimeDelta kRetryAfter = base::Minutes(1);
}  // namespace

RedeemUnblindedPaymentTokens::RedeemUnblindedPaymentTokens() = default;

RedeemUnblindedPaymentTokens::~RedeemUnblindedPaymentTokens() {
  delegate_ = nullptr;
}

void RedeemUnblindedPaymentTokens::MaybeRedeemAfterDelay(
    const WalletInfo& wallet) {
  CHECK(wallet.IsValid());

  if (is_processing_ || timer_.IsRunning() || retry_timer_.IsRunning()) {
    return;
  }

  wallet_ = wallet;

  const base::Time redeem_at =
      timer_.Start(FROM_HERE, CalculateDelayBeforeRedeemingTokens(),
                   base::BindOnce(&RedeemUnblindedPaymentTokens::Redeem,
                                  base::Unretained(this)));
  SetNextTokenRedemptionAt(redeem_at);

  BLOG(1, "Redeem unblinded payment tokens " << FriendlyDateAndTime(redeem_at));
}

///////////////////////////////////////////////////////////////////////////////

void RedeemUnblindedPaymentTokens::Redeem() {
  CHECK(!is_processing_);

  BLOG(1, "RedeemUnblindedPaymentTokens");

  if (privacy::UnblindedPaymentTokensIsEmpty()) {
    BLOG(1, "No unblinded payment tokens to redeem");
    return ScheduleNextRedemption();
  }

  BLOG(2, "PUT /v3/confirmation/payment/{paymentId}");

  is_processing_ = true;

  const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens =
      privacy::GetAllUnblindedPaymentTokens();

  BuildRedeemUnblindedPaymentTokensUserData(
      unblinded_payment_tokens,
      base::BindOnce(&RedeemUnblindedPaymentTokens::BuildUserDataCallback,
                     weak_factory_.GetWeakPtr()));
}

void RedeemUnblindedPaymentTokens::BuildUserDataCallback(
    base::Value::Dict user_data) {
  const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens =
      privacy::GetAllUnblindedPaymentTokens();

  RedeemUnblindedPaymentTokensUrlRequestBuilder url_request_builder(
      wallet_, unblinded_payment_tokens, std::move(user_data));
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  AdsClientHelper::GetInstance()->UrlRequest(
      std::move(url_request),
      base::BindOnce(&RedeemUnblindedPaymentTokens::RedeemCallback,
                     weak_factory_.GetWeakPtr(), unblinded_payment_tokens));
}

void RedeemUnblindedPaymentTokens::RedeemCallback(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens,
    const mojom::UrlResponseInfo& url_response) {
  BLOG(1, "OnRedeemUnblindedPaymentTokens");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code != net::HTTP_OK) {
    return FailedToRedeem(/*should_retry*/ true);
  }

  SuccessfullyRedeemed(unblinded_payment_tokens);
}

void RedeemUnblindedPaymentTokens::SuccessfullyRedeemed(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens) {
  BLOG(1, "Successfully redeemed unblinded payment tokens");

  is_processing_ = false;

  StopRetrying();

  privacy::RemoveUnblindedPaymentTokens(unblinded_payment_tokens);

  if (delegate_) {
    delegate_->OnDidRedeemUnblindedPaymentTokens(unblinded_payment_tokens);
  }

  ScheduleNextRedemption();
}

void RedeemUnblindedPaymentTokens::FailedToRedeem(const bool should_retry) {
  BLOG(1, "Failed to redeem unblinded payment tokens");

  if (delegate_) {
    delegate_->OnFailedToRedeemUnblindedPaymentTokens();
  }

  if (should_retry) {
    Retry();
  }
}

void RedeemUnblindedPaymentTokens::ScheduleNextRedemption() {
  const base::Time redeem_at = CalculateNextTokenRedemptionAt();

  SetNextTokenRedemptionAt(redeem_at);

  if (delegate_) {
    delegate_->OnDidScheduleNextUnblindedPaymentTokenRedemption(redeem_at);
  }

  MaybeRedeemAfterDelay(wallet_);
}

void RedeemUnblindedPaymentTokens::Retry() {
  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&RedeemUnblindedPaymentTokens::RetryCallback,
                     weak_factory_.GetWeakPtr()));

  BLOG(1, "Retry redeeming unblinded payment tokens "
              << FriendlyDateAndTime(retry_at));

  if (delegate_) {
    delegate_->OnWillRetryRedeemingUnblindedPaymentTokens(retry_at);
  }
}

void RedeemUnblindedPaymentTokens::RetryCallback() {
  BLOG(1, "Retry redeeming unblinded payment tokens");

  if (delegate_) {
    delegate_->OnDidRetryRedeemingUnblindedPaymentTokens();
  }

  is_processing_ = false;

  Redeem();
}

void RedeemUnblindedPaymentTokens::StopRetrying() {
  retry_timer_.Stop();
}

}  // namespace brave_ads
