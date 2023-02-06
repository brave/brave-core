/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"

#include <cstdint>
#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_url_request_builder.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_user_data_builder.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/common/time/time_formatting_util.h"
#include "bat/ads/internal/common/url/url_request_string_util.h"
#include "bat/ads/internal/common/url/url_response_string_util.h"
#include "bat/ads/internal/flags/flag_manager.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"

namespace ads {

namespace {

constexpr base::TimeDelta kRetryAfter = base::Minutes(1);
constexpr base::TimeDelta kExpiredNextTokenRedemptionAfter = base::Minutes(1);
constexpr int64_t kNextTokenRedemptionAfterSeconds =
    24 * base::Time::kSecondsPerHour;
constexpr int64_t kDebugNextTokenRedemptionAfterSeconds =
    25 * base::Time::kSecondsPerMinute;

base::TimeDelta CalculateTokenRedemptionDelay() {
  const base::Time next_token_redemption_at =
      AdsClientHelper::GetInstance()->GetTimePref(
          prefs::kNextTokenRedemptionAt);

  const base::Time now = base::Time::Now();

  base::TimeDelta delay;
  if (now >= next_token_redemption_at) {
    // Browser was launched after the next token redemption date
    delay = kExpiredNextTokenRedemptionAfter;
  } else {
    delay = next_token_redemption_at - now;
  }

  return delay;
}

base::Time CalculateNextTokenRedemptionDate() {
  const base::Time now = base::Time::Now();

  const int64_t delay = FlagManager::GetInstance()->ShouldDebug()
                            ? kDebugNextTokenRedemptionAfterSeconds
                            : kNextTokenRedemptionAfterSeconds;

  const auto rand_delay =
      static_cast<int64_t>(brave_base::random::Geometric(delay));

  return now + base::Seconds(rand_delay);
}

}  // namespace

RedeemUnblindedPaymentTokens::RedeemUnblindedPaymentTokens() = default;

RedeemUnblindedPaymentTokens::~RedeemUnblindedPaymentTokens() {
  delegate_ = nullptr;
}

void RedeemUnblindedPaymentTokens::MaybeRedeemAfterDelay(
    const WalletInfo& wallet) {
  if (is_processing_ || timer_.IsRunning() || retry_timer_.IsRunning()) {
    return;
  }

  if (!wallet.IsValid()) {
    BLOG(0, "Failed to redeem unblinded payment tokens due to invalid wallet");
    FailedToRedeemUnblindedPaymentTokens(/*should_retry*/ false);
    return;
  }

  wallet_ = wallet;

  const base::Time redeem_at =
      timer_.Start(FROM_HERE, CalculateTokenRedemptionDelay(),
                   base::BindOnce(&RedeemUnblindedPaymentTokens::Redeem,
                                  base::Unretained(this)));

  BLOG(1, "Redeem unblinded payment tokens "
              << FriendlyDateAndTime(redeem_at, /*use_sentence_style*/ true));
}

///////////////////////////////////////////////////////////////////////////////

void RedeemUnblindedPaymentTokens::Redeem() {
  DCHECK(!is_processing_);

  BLOG(1, "RedeemUnblindedPaymentTokens");

  if (privacy::UnblindedPaymentTokensIsEmpty()) {
    BLOG(1, "No unblinded payment tokens to redeem");
    ScheduleNextTokenRedemption();
    return;
  }

  BLOG(2, "PUT /v3/confirmation/payment/{paymentId}");

  is_processing_ = true;

  const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens =
      privacy::GetAllUnblindedPaymentTokens();

  const RedeemUnblindedPaymentTokensUserDataBuilder user_data_builder(
      unblinded_payment_tokens);
  user_data_builder.Build(
      base::BindOnce(&RedeemUnblindedPaymentTokens::
                         OnRedeemUnblindedPaymentTokensUserDataBuilt,
                     base::Unretained(this)));
}

void RedeemUnblindedPaymentTokens::OnRedeemUnblindedPaymentTokensUserDataBuilt(
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
      base::BindOnce(&RedeemUnblindedPaymentTokens::OnRedeem,
                     base::Unretained(this), unblinded_payment_tokens));
}

void RedeemUnblindedPaymentTokens::OnRedeem(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens,
    const mojom::UrlResponseInfo& url_response) {
  BLOG(1, "OnRedeemUnblindedPaymentTokens");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code != net::HTTP_OK) {
    FailedToRedeemUnblindedPaymentTokens(/*should_retry*/ true);
    return;
  }

  SuccessfullyRedeemedUnblindedPaymentTokens(unblinded_payment_tokens);
}

void RedeemUnblindedPaymentTokens::SuccessfullyRedeemedUnblindedPaymentTokens(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens) {
  BLOG(1, "Successfully redeemed unblinded payment tokens");

  is_processing_ = false;

  retry_timer_.Stop();

  privacy::RemoveUnblindedPaymentTokens(unblinded_payment_tokens);

  if (delegate_) {
    delegate_->OnDidRedeemUnblindedPaymentTokens(unblinded_payment_tokens);
  }

  ScheduleNextTokenRedemption();
}

void RedeemUnblindedPaymentTokens::FailedToRedeemUnblindedPaymentTokens(
    const bool should_retry) {
  BLOG(1, "Failed to redeem unblinded payment tokens");

  if (delegate_) {
    delegate_->OnFailedToRedeemUnblindedPaymentTokens();
  }

  if (should_retry) {
    Retry();
  }
}

void RedeemUnblindedPaymentTokens::ScheduleNextTokenRedemption() {
  const base::Time redeem_at = CalculateNextTokenRedemptionDate();

  AdsClientHelper::GetInstance()->SetTimePref(prefs::kNextTokenRedemptionAt,
                                              redeem_at);

  if (delegate_) {
    delegate_->OnDidScheduleNextUnblindedPaymentTokensRedemption(redeem_at);
  }

  MaybeRedeemAfterDelay(wallet_);
}

void RedeemUnblindedPaymentTokens::Retry() {
  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&RedeemUnblindedPaymentTokens::OnRetry,
                     base::Unretained(this)));

  BLOG(1, "Retry redeeming unblinded payment tokens "
              << FriendlyDateAndTime(retry_at, /*use_sentence_style*/ true));

  if (delegate_) {
    delegate_->OnWillRetryRedeemingUnblindedPaymentTokens(retry_at);
  }
}

void RedeemUnblindedPaymentTokens::OnRetry() {
  BLOG(1, "Retry redeeming unblinded payment tokens");

  if (delegate_) {
    delegate_->OnDidRetryRedeemingUnblindedPaymentTokens();
  }

  is_processing_ = false;

  Redeem();
}

}  // namespace ads
