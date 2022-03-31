/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"

#include <cstdint>
#include <functional>
#include <utility>

#include "base/check.h"
#include "base/time/time.h"
#include "bat/ads/ads.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/account/confirmations/confirmations_state.h"
#include "bat/ads/internal/account/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_url_request_builder.h"
#include "bat/ads/internal/account/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_user_data_builder.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/logging_util.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_tokens.h"
#include "bat/ads/internal/time_formatting_util.h"
#include "bat/ads/pref_names.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"

namespace ads {

namespace {

constexpr int64_t kRetryAfterSeconds = 1 * base::Time::kSecondsPerMinute;

constexpr int64_t kNextTokenRedemptionAfterSeconds =
    24 * base::Time::kSecondsPerHour;
constexpr int64_t kDebugNextTokenRedemptionAfterSeconds =
    25 * base::Time::kSecondsPerMinute;
constexpr int64_t kExpiredNextTokenRedemptionAfterSeconds =
    1 * base::Time::kSecondsPerMinute;

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

    if (delegate_) {
      delegate_->OnFailedToRedeemUnblindedPaymentTokens();
    }

    return;
  }

  wallet_ = wallet;

  const base::TimeDelta delay = CalculateTokenRedemptionDelay();

  const base::Time time =
      timer_.Start(delay, base::BindOnce(&RedeemUnblindedPaymentTokens::Redeem,
                                         base::Unretained(this)));

  BLOG(1, "Redeem unblinded payment tokens " << FriendlyDateAndTime(time));
}

///////////////////////////////////////////////////////////////////////////////

void RedeemUnblindedPaymentTokens::Redeem() {
  DCHECK(!is_processing_);

  BLOG(1, "RedeemUnblindedPaymentTokens");

  if (ConfirmationsState::Get()->get_unblinded_payment_tokens()->IsEmpty()) {
    BLOG(1, "No unblinded payment tokens to redeem");

    ScheduleNextTokenRedemption();
    return;
  }

  BLOG(2, "PUT /v2/confirmation/payment/{payment_id}");

  is_processing_ = true;

  const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens =
      ConfirmationsState::Get()->get_unblinded_payment_tokens()->GetAllTokens();

  RedeemUnblindedPaymentTokensUserDataBuilder user_data_builder(
      unblinded_payment_tokens);
  user_data_builder.Build([=](const base::Value& user_data) {
    RedeemUnblindedPaymentTokensUrlRequestBuilder url_request_builder(
        wallet_, unblinded_payment_tokens, user_data);
    mojom::UrlRequestPtr url_request = url_request_builder.Build();
    BLOG(6, UrlRequestToString(url_request));
    BLOG(7, UrlRequestHeadersToString(url_request));

    const auto callback =
        std::bind(&RedeemUnblindedPaymentTokens::OnRedeem, this,
                  std::placeholders::_1, unblinded_payment_tokens);
    AdsClientHelper::Get()->UrlRequest(std::move(url_request), callback);
  });
}

void RedeemUnblindedPaymentTokens::OnRedeem(
    const mojom::UrlResponse& url_response,
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens) {
  BLOG(1, "OnRedeemUnblindedPaymentTokens");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to redeem unblinded payment tokens");
    OnFailedToRedeemUnblindedPaymentTokens();
    return;
  }

  OnDidRedeemUnblindedPaymentTokens(unblinded_payment_tokens);
}

void RedeemUnblindedPaymentTokens::OnDidRedeemUnblindedPaymentTokens(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens) {
  is_processing_ = false;

  retry_timer_.Stop();

  ConfirmationsState::Get()->get_unblinded_payment_tokens()->RemoveTokens(
      unblinded_payment_tokens);
  ConfirmationsState::Get()->Save();

  if (delegate_) {
    delegate_->OnDidRedeemUnblindedPaymentTokens(unblinded_payment_tokens);
  }

  ScheduleNextTokenRedemption();
}

void RedeemUnblindedPaymentTokens::OnFailedToRedeemUnblindedPaymentTokens() {
  if (delegate_) {
    delegate_->OnFailedToRedeemUnblindedPaymentTokens();
  }

  Retry();
}

void RedeemUnblindedPaymentTokens::ScheduleNextTokenRedemption() {
  const base::Time next_token_redemption_at =
      CalculateNextTokenRedemptionDate();

  AdsClientHelper::Get()->SetDoublePref(prefs::kNextTokenRedemptionAt,
                                        next_token_redemption_at.ToDoubleT());

  if (delegate_) {
    delegate_->OnDidScheduleNextUnblindedPaymentTokensRedemption(
        next_token_redemption_at);
  }

  MaybeRedeemAfterDelay(wallet_);
}

void RedeemUnblindedPaymentTokens::Retry() {
  if (delegate_) {
    delegate_->OnWillRetryRedeemingUnblindedPaymentTokens();
  }

  const base::Time time = retry_timer_.StartWithPrivacy(
      base::Seconds(kRetryAfterSeconds),
      base::BindOnce(&RedeemUnblindedPaymentTokens::OnRetry,
                     base::Unretained(this)));

  BLOG(1, "Retry redeeming unblinded payment tokens "
              << FriendlyDateAndTime(time));
}

void RedeemUnblindedPaymentTokens::OnRetry() {
  if (delegate_) {
    delegate_->OnDidRetryRedeemingUnblindedPaymentTokens();
  }

  is_processing_ = false;

  Redeem();
}

base::TimeDelta RedeemUnblindedPaymentTokens::CalculateTokenRedemptionDelay() {
  const base::Time next_token_redemption_at = base::Time::FromDoubleT(
      AdsClientHelper::Get()->GetDoublePref(prefs::kNextTokenRedemptionAt));

  const base::Time now = base::Time::Now();

  base::TimeDelta delay;
  if (now >= next_token_redemption_at) {
    // Browser was launched after the next token redemption date
    delay = base::Seconds(kExpiredNextTokenRedemptionAfterSeconds);
  } else {
    delay = next_token_redemption_at - now;
  }

  return delay;
}

base::Time RedeemUnblindedPaymentTokens::CalculateNextTokenRedemptionDate() {
  const base::Time now = base::Time::Now();

  int64_t delay;

  if (!g_is_debug) {
    delay = kNextTokenRedemptionAfterSeconds;
  } else {
    delay = kDebugNextTokenRedemptionAfterSeconds;
  }

  const int64_t rand_delay =
      static_cast<int64_t>(brave_base::random::Geometric(delay));

  return now + base::Seconds(rand_delay);
}

}  // namespace ads
