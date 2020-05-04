/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/payout_tokens.h"
#include "bat/confirmations/internal/static_values.h"
#include "bat/confirmations/internal/logging.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/unblinded_tokens.h"
#include "bat/confirmations/internal/redeem_payment_tokens_request.h"
#include "bat/confirmations/internal/time_util.h"

#include "brave_base/random.h"
#include "net/http/http_status_code.h"
#include "base/time/time.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace confirmations {

PayoutTokens::PayoutTokens(
    ConfirmationsImpl* confirmations,
    UnblindedTokens* unblinded_payment_tokens)
    : confirmations_(confirmations),
      unblinded_payment_tokens_(unblinded_payment_tokens) {
}

PayoutTokens::~PayoutTokens() = default;

void PayoutTokens::PayoutAfterDelay(
    const WalletInfo& wallet_info) {
  if (retry_timer_.IsRunning()) {
    return;
  }

  wallet_info_ = wallet_info;
  if (!wallet_info_.IsValid()) {
    BLOG(0, "Failed to payout tokens due to invalid wallet");
    return;
  }

  const uint64_t delay = CalculatePayoutDelay();

  const base::Time time = timer_.Start(delay,
      base::BindOnce(&PayoutTokens::RedeemPaymentTokens,
          base::Unretained(this)));

  BLOG(1, "Payout tokens " << FriendlyDateAndTime(time));
}

uint64_t PayoutTokens::get_token_redemption_timestamp_in_seconds() const {
  return token_redemption_timestamp_in_seconds_;
}

void PayoutTokens::set_token_redemption_timestamp_in_seconds(
    const uint64_t timestamp_in_seconds) {
  token_redemption_timestamp_in_seconds_ = timestamp_in_seconds;
}

///////////////////////////////////////////////////////////////////////////////

void PayoutTokens::RedeemPaymentTokens() {
  BLOG(1, "RedeemPaymentTokens");

  if (unblinded_payment_tokens_->IsEmpty()) {
    BLOG(1, "No unblinded payment tokens to redeem");
    ScheduleNextPayout();
    return;
  }

  BLOG(1, "PUT /v1/confirmation/payment/{payment_id}");
  RedeemPaymentTokensRequest request;

  auto tokens = unblinded_payment_tokens_->GetAllTokens();

  auto payload = request.CreatePayload(wallet_info_);

  auto url = request.BuildUrl(wallet_info_);
  auto method = request.GetMethod();
  auto body = request.BuildBody(tokens, payload);
  auto headers = request.BuildHeaders();
  auto content_type = request.GetContentType();

  auto callback = std::bind(&PayoutTokens::OnRedeemPaymentTokens, this, _1);

  BLOG(5, UrlRequestToString(url, headers, body, content_type, method));
  confirmations_->get_client()->LoadURL(url, headers, body, content_type,
      method, callback);
}

void PayoutTokens::OnRedeemPaymentTokens(
    const UrlResponse& url_response) {
  BLOG(1, "OnRedeemPaymentTokens");

  BLOG(6, UrlResponseToString(url_response));

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to redeem payment tokens");
    OnPayout(FAILED);
    return;
  }

  OnPayout(SUCCESS);
}

void PayoutTokens::OnPayout(const Result result) {
  if (result != SUCCESS) {
    BLOG(1, "Failed to payout tokens");

    const base::Time time = retry_timer_.StartWithBackoff(
        kRetryPayoutTokensAfterSeconds, base::BindOnce(&PayoutTokens::OnRetry,
            base::Unretained(this)));

    BLOG(1, "Retry paying out tokens " << FriendlyDateAndTime(time));

    return;
  }

  BLOG(1, "Successfully paid out tokens");

  confirmations_->AddUnredeemedTransactionsToPendingRewards();
  unblinded_payment_tokens_->RemoveAllTokens();

  confirmations_->UpdateAdsRewards(true);

  retry_timer_.Stop();

  ScheduleNextPayout();
}

void PayoutTokens::ScheduleNextPayout() {
  UpdateNextTokenRedemptionDate();
  PayoutAfterDelay(wallet_info_);
}

void PayoutTokens::OnRetry() {
  BLOG(1, "Retry paying out tokens");

  RedeemPaymentTokens();
}

uint64_t PayoutTokens::CalculatePayoutDelay() {
  if (token_redemption_timestamp_in_seconds_ == 0) {
    UpdateNextTokenRedemptionDate();
  }

  const uint64_t now_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  uint64_t delay;
  if (now_in_seconds >= token_redemption_timestamp_in_seconds_) {
    // Browser was launched after the token redemption date
    delay = 1 * base::Time::kSecondsPerMinute;
  } else {
    delay = token_redemption_timestamp_in_seconds_ - now_in_seconds;
  }

  return delay;
}

void PayoutTokens::UpdateNextTokenRedemptionDate() {
  const uint64_t now_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  uint64_t delay;

  if (!_is_debug) {
    delay = kNextTokenRedemptionAfterSeconds;
  } else {
    delay = kDebugNextTokenRedemptionAfterSeconds;
  }

  const uint64_t rand_delay = brave_base::random::Geometric(delay);

  token_redemption_timestamp_in_seconds_ = now_in_seconds + rand_delay;
  confirmations_->SaveState();
}

}  // namespace confirmations
