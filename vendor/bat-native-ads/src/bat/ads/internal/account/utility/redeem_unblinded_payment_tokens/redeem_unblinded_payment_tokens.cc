/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"

#include <cstdint>
#include <functional>
#include <utility>

#include "base/time/time.h"
#include "bat/ads/ads.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_url_request_builder.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_user_data_builder.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/http_status_code.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/time_formatting_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmations_state.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens.h"
#include "bat/ads/internal/server/url/url_request_string_util.h"
#include "bat/ads/internal/server/url/url_response_string_util.h"
#include "bat/ads/pref_names.h"
#include "brave_base/random.h"

namespace ads {

namespace {

constexpr base::TimeDelta kRetryAfter = base::Minutes(1);
constexpr base::TimeDelta kExpiredNextTokenRedemptionAfter = base::Minutes(1);
constexpr int64_t kNextTokenRedemptionAfterSeconds =
    24 * base::Time::kSecondsPerHour;
constexpr int64_t kDebugNextTokenRedemptionAfterSeconds =
    25 * base::Time::kSecondsPerMinute;

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

  if (url_response.status_code == net::HTTP_UPGRADE_REQUIRED) {
    BLOG(1,
         "Failed to redeem unblinded payment token as a browser upgrade is "
         "required");
    OnFailedToRedeemUnblindedPaymentTokens();
    return;
  } else if (url_response.status_code != net::HTTP_OK) {
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
  const base::Time redeem_at = CalculateNextTokenRedemptionDate();

  AdsClientHelper::Get()->SetTimePref(prefs::kNextTokenRedemptionAt, redeem_at);

  if (delegate_) {
    delegate_->OnDidScheduleNextUnblindedPaymentTokensRedemption(redeem_at);
  }

  MaybeRedeemAfterDelay(wallet_);
}

void RedeemUnblindedPaymentTokens::Retry() {
  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      kRetryAfter, base::BindOnce(&RedeemUnblindedPaymentTokens::OnRetry,
                                  base::Unretained(this)));

  if (delegate_) {
    delegate_->OnWillRetryRedeemingUnblindedPaymentTokens(retry_at);
  }

  BLOG(1, "Retry redeeming unblinded payment tokens "
              << FriendlyDateAndTime(retry_at));
}

void RedeemUnblindedPaymentTokens::OnRetry() {
  if (delegate_) {
    delegate_->OnDidRetryRedeemingUnblindedPaymentTokens();
  }

  is_processing_ = false;

  Redeem();
}

base::TimeDelta RedeemUnblindedPaymentTokens::CalculateTokenRedemptionDelay() {
  const base::Time next_token_redemption_at =
      AdsClientHelper::Get()->GetTimePref(prefs::kNextTokenRedemptionAt);

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
