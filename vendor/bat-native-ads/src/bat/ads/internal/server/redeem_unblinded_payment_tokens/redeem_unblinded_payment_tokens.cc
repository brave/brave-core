/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"

#include <stdint.h>

#include <functional>
#include <utility>

#include "brave_base/random.h"
#include "net/http/http_status_code.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"
#include "bat/ads/internal/server/ad_rewards/ad_rewards.h"
#include "bat/ads/internal/server/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_url_request_builder.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

using std::placeholders::_1;

namespace {

const uint64_t kRetryAfterSeconds = 1 * base::Time::kSecondsPerMinute;

const uint64_t kNextTokenRedemptionAfterSeconds =
    24 * base::Time::kSecondsPerHour;
const uint64_t kDebugNextTokenRedemptionAfterSeconds =
    25 * base::Time::kSecondsPerMinute;

}  // namespace

RedeemUnblindedPaymentTokens::RedeemUnblindedPaymentTokens(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

RedeemUnblindedPaymentTokens::~RedeemUnblindedPaymentTokens() = default;

void RedeemUnblindedPaymentTokens::set_delegate(
    RedeemUnblindedPaymentTokensDelegate* delegate) {
  DCHECK(delegate);

  delegate_ = delegate;
}

void RedeemUnblindedPaymentTokens::RedeemAfterDelay(
    const WalletInfo& wallet) {
  if (retry_timer_.IsRunning()) {
    return;
  }

  wallet_ = wallet;
  if (!wallet_.IsValid()) {
    BLOG(0, "Failed to redeem unblinded payment tokens due to invalid wallet");
    return;
  }

  const base::TimeDelta delay = CalculateTokenRedemptionDelay();

  const base::Time time = timer_.Start(delay,
      base::BindOnce(&RedeemUnblindedPaymentTokens::Redeem,
          base::Unretained(this)));

  BLOG(1, "Redeem unblinded payment tokens " << FriendlyDateAndTime(time));
}

///////////////////////////////////////////////////////////////////////////////

void RedeemUnblindedPaymentTokens::Redeem() {
  BLOG(1, "RedeemUnblindedPaymentTokens");

  if (ads_->get_confirmations()->get_unblinded_payment_tokens()->IsEmpty()) {
    BLOG(1, "No unblinded payment tokens to redeem");
    ScheduleNextTokenRedemption();
    return;
  }

  BLOG(2, "PUT /v1/confirmation/payment/{payment_id}");

  const privacy::UnblindedTokenList unblinded_tokens =
      ads_->get_confirmations()->get_unblinded_payment_tokens()->GetAllTokens();
  RedeemUnblindedPaymentTokensUrlRequestBuilder
      url_request_builder(wallet_, unblinded_tokens);
  UrlRequestPtr url_request = url_request_builder.Build();
  BLOG(5, UrlRequestToString(url_request));

  auto callback = std::bind(&RedeemUnblindedPaymentTokens::OnRedeem, this, _1);
  ads_->get_ads_client()->UrlRequest(std::move(url_request), callback);
}

void RedeemUnblindedPaymentTokens::OnRedeem(
    const UrlResponse& url_response) {
  BLOG(1, "OnRedeemUnblindedPaymentTokens");

  BLOG(6, UrlResponseToString(url_response));

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to redeem unblinded payment tokens");
    OnRedeemUnblindedPaymentTokens(FAILED);
    return;
  }

  OnRedeemUnblindedPaymentTokens(SUCCESS);
}

void RedeemUnblindedPaymentTokens::OnRedeemUnblindedPaymentTokens(
    const Result result) {
  if (result != SUCCESS) {
    if (delegate_) {
      delegate_->OnFailedToRedeemUnblindedPaymentTokens();
    }

    const base::Time time = retry_timer_.StartWithPrivacy(
        base::TimeDelta::FromSeconds(kRetryAfterSeconds),
            base::BindOnce(&RedeemUnblindedPaymentTokens::OnRetry,
                base::Unretained(this)));

    BLOG(1, "Retry redeeming unblinded payment tokens "
        << FriendlyDateAndTime(time));

    return;
  }

  const TransactionList unredeemed_transactions =
      ads_->GetUnredeemedTransactions();
  ads_->get_ad_rewards()->SetUnreconciledTransactions(unredeemed_transactions);

  ads_->get_confirmations()->get_unblinded_payment_tokens()->RemoveAllTokens();

  retry_timer_.Stop();

  ScheduleNextTokenRedemption();

  if (delegate_) {
    delegate_->OnDidRedeemUnblindedPaymentTokens();
  }

  ads_->UpdateAdRewards(/*should_reconcile*/ true);
}

void RedeemUnblindedPaymentTokens::ScheduleNextTokenRedemption() {
  const base::Time next_token_redemption_date =
      CalculateNextTokenRedemptionDate();

  ads_->get_confirmations()->set_next_token_redemption_date(
      next_token_redemption_date);

  RedeemAfterDelay(wallet_);
}

void RedeemUnblindedPaymentTokens::OnRetry() {
  if (delegate_) {
    delegate_->OnDidRetryRedeemingUnblindedPaymentTokens();
  }

  Redeem();
}

base::TimeDelta RedeemUnblindedPaymentTokens::CalculateTokenRedemptionDelay() {
  base::Time next_token_redemption_date =
      ads_->get_confirmations()->get_next_token_redemption_date();

  if (next_token_redemption_date.is_null()) {
    next_token_redemption_date = CalculateNextTokenRedemptionDate();

    ads_->get_confirmations()->set_next_token_redemption_date(
        next_token_redemption_date);
  }

  const base::Time now = base::Time::Now();

  base::TimeDelta delay;
  if (now >= next_token_redemption_date) {
    // Browser was launched after the next token redemption date
    delay = base::TimeDelta::FromMinutes(1);
  } else {
    delay = next_token_redemption_date - now;
  }

  return delay;
}

base::Time RedeemUnblindedPaymentTokens::CalculateNextTokenRedemptionDate() {
  const base::Time now = base::Time::Now();

  uint64_t delay;

  if (!_is_debug) {
    delay = kNextTokenRedemptionAfterSeconds;
  } else {
    delay = kDebugNextTokenRedemptionAfterSeconds;
  }

  const uint64_t rand_delay = brave_base::random::Geometric(delay);

  return now + base::TimeDelta::FromSeconds(rand_delay);
}

}  // namespace ads
