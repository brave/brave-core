/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/ad_rewards/ad_rewards.h"

#include <functional>
#include <utility>

#include "base/time/time.h"
#include "bat/ads/internal/account/ad_rewards/payments/payments.h"
#include "bat/ads/internal/account/ad_rewards/payments/payments_url_request_builder.h"
#include "bat/ads/internal/account/confirmations/confirmations_state.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_formatting_util.h"
#include "bat/ads/transaction_info.h"
#include "net/http/http_status_code.h"

namespace ads {

namespace {
const int64_t kRetryAfterSeconds = 1 * base::Time::kSecondsPerMinute;
}  // namespace

AdRewards::AdRewards() : payments_(std::make_unique<Payments>()) {}

AdRewards::~AdRewards() = default;

void AdRewards::set_delegate(AdRewardsDelegate* delegate) {
  delegate_ = delegate;
}

void AdRewards::MaybeReconcile(const WalletInfo& wallet) {
  if (is_processing_ || retry_timer_.IsRunning()) {
    return;
  }

  if (!wallet.IsValid()) {
    BLOG(0, "Failed to reconcile ad rewards due to invalid wallet");
    return;
  }

  wallet_ = wallet;

  Reconcile();
}

uint64_t AdRewards::GetNextPaymentDate() const {
  const base::Time now = base::Time::Now();

  const base::Time next_token_redemption_date =
      ConfirmationsState::Get()->get_next_token_redemption_date();

  const base::Time next_payment_date =
      payments_->CalculateNextPaymentDate(now, next_token_redemption_date);

  return static_cast<uint64_t>(next_payment_date.ToDoubleT());
}

uint64_t AdRewards::GetAdsReceivedThisMonth() const {
  const base::Time now = base::Time::Now();
  return GetAdsReceivedForMonth(now);
}

uint64_t AdRewards::GetAdsReceivedForMonth(const base::Time& time) const {
  const TransactionList transactions =
      ConfirmationsState::Get()->get_transactions();

  uint64_t ads_received_this_month = 0;

  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);

  for (const auto& transaction : transactions) {
    if (transaction.timestamp == 0) {
      // Workaround for Windows crash when passing 0 to UTCExplode
      continue;
    }

    const base::Time transaction_time =
        base::Time::FromDoubleT(transaction.timestamp);

    base::Time::Exploded transaction_time_exploded;
    transaction_time.LocalExplode(&transaction_time_exploded);

    if (transaction_time_exploded.year == exploded.year &&
        transaction_time_exploded.month == exploded.month &&
        transaction.estimated_redemption_value > 0.0 &&
        ConfirmationType(transaction.confirmation_type) ==
            ConfirmationType::kViewed) {
      ads_received_this_month++;
    }
  }

  return ads_received_this_month;
}

double AdRewards::GetEarningsForThisMonth() const {
  const base::Time now = base::Time::Now();
  double earnings_for_this_month = GetEarningsForMonth(now);
  earnings_for_this_month += GetUnclearedEarningsForThisMonth();
  return earnings_for_this_month;
}

double AdRewards::GetEarningsForMonth(const base::Time& time) const {
  const PaymentInfo payment = payments_->GetForThisMonth(time);
  return payment.balance;
}

double AdRewards::GetUnclearedEarningsForThisMonth() const {
  const base::Time now = base::Time::Now();
  base::Time::Exploded exploded;
  now.UTCExplode(&exploded);

  exploded.day_of_month = 1;
  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;

  base::Time from_time;
  const bool success = base::Time::FromUTCExploded(exploded, &from_time);
  DCHECK(success);

  const int64_t from_timestamp = static_cast<int64_t>(from_time.ToDoubleT());

  const int64_t to_timestamp =
      static_cast<int64_t>(base::Time::Now().ToDoubleT());

  const TransactionList uncleared_transactions = transactions::GetUncleared();

  return CalculateEarningsForTransactions(uncleared_transactions,
                                          from_timestamp, to_timestamp);
}

base::Value AdRewards::GetAsDictionary() {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  base::Value payments = payments_->GetAsList();
  dictionary.SetKey("payments", base::Value(std::move(payments)));

  return dictionary;
}

bool AdRewards::SetFromDictionary(base::Value* dictionary) {
  DCHECK(dictionary);

  if (!payments_->SetFromDictionary(dictionary)) {
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

void AdRewards::Reconcile() {
  DCHECK(!is_processing_);

  BLOG(1, "Reconcile ad rewards");

  is_processing_ = true;

  GetPayments();
}

void AdRewards::GetPayments() {
  BLOG(1, "GetPayments");
  BLOG(2, "GET /v1/confirmation/payment/{payment_id}");

  PaymentsUrlRequestBuilder url_request_builder(wallet_);
  UrlRequestPtr url_request = url_request_builder.Build();
  BLOG(5, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  auto callback =
      std::bind(&AdRewards::OnGetPayments, this, std::placeholders::_1);
  AdsClientHelper::Get()->UrlRequest(std::move(url_request), callback);
}

void AdRewards::OnGetPayments(const UrlResponse& url_response) {
  BLOG(1, "OnGetPayments");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to get payment balance");
    OnFailedToReconcileAdRewards();
    return;
  }

  if (!payments_->SetFromJson(url_response.body)) {
    BLOG(0, "Failed to parse payment balance");
    BLOG(6, "Payment balance response body: " << url_response.body);
    OnFailedToReconcileAdRewards();
    return;
  }

  OnDidReconcileAdRewards();
}

void AdRewards::OnDidReconcileAdRewards() {
  is_processing_ = false;

  BLOG(1, "Successfully reconciled ad rewards");

  retry_timer_.Stop();

  if (delegate_) {
    delegate_->OnDidReconcileAdRewards();
  }
}

void AdRewards::OnFailedToReconcileAdRewards() {
  is_processing_ = false;

  BLOG(1, "Failed to reconcile ad rewards");

  Retry();

  if (!delegate_) {
    return;
  }

  delegate_->OnFailedToReconcileAdRewards();
}

void AdRewards::Retry() {
  if (delegate_) {
    delegate_->OnWillRetryToReconcileAdRewards();
  }

  const base::Time time = retry_timer_.StartWithPrivacy(
      base::TimeDelta::FromSeconds(kRetryAfterSeconds),
      base::BindOnce(&AdRewards::OnRetry, base::Unretained(this)));

  BLOG(1, "Retry reconciling ad rewards " << FriendlyDateAndTime(time));
}

void AdRewards::OnRetry() {
  BLOG(1, "Retry reconciling ad rewards");

  if (delegate_) {
    delegate_->OnDidRetryToReconcileAdRewards();
  }

  Reconcile();
}

}  // namespace ads
