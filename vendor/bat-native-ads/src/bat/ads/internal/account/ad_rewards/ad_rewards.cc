/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/ad_rewards/ad_rewards.h"

#include <functional>
#include <utility>

#include "base/time/time.h"
#include "bat/ads/internal/account/ad_rewards/ad_grants/ad_grants.h"
#include "bat/ads/internal/account/ad_rewards/ad_grants/ad_grants_url_request_builder.h"
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

double CalculateEarningsForTransactions(const TransactionList& transactions,
                                        const int64_t from_timestamp,
                                        const int64_t to_timestamp) {
  double estimated_pending_rewards = 0.0;

  for (const auto& transaction : transactions) {
    if (transaction.timestamp < from_timestamp ||
        transaction.timestamp > to_timestamp) {
      continue;
    }

    estimated_pending_rewards += transaction.estimated_redemption_value;
  }

  return estimated_pending_rewards;
}

}  // namespace

AdRewards::AdRewards()
    : ad_grants_(std::make_unique<AdGrants>()),
      payments_(std::make_unique<Payments>()) {}

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

double AdRewards::GetEstimatedPendingRewards() const {
  double estimated_pending_rewards = payments_->GetBalance();

  estimated_pending_rewards -= ad_grants_->GetBalance();

  const int64_t to_timestamp =
      static_cast<int64_t>(base::Time::Now().ToDoubleT());
  const TransactionList uncleared_transactions = transactions::GetUncleared();
  const double uncleared_estimated_pending_rewards =
      CalculateEarningsForTransactions(uncleared_transactions, 0, to_timestamp);
  estimated_pending_rewards += uncleared_estimated_pending_rewards;

  estimated_pending_rewards += unreconciled_estimated_pending_rewards_;

  if (estimated_pending_rewards < 0.0) {
    estimated_pending_rewards = 0.0;
  }

  return estimated_pending_rewards;
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

void AdRewards::SetUnreconciledTransactions(
    const TransactionList& unreconciled_transactions) {
  const int64_t to_timestamp =
      static_cast<int64_t>(base::Time::Now().ToDoubleT());

  unreconciled_estimated_pending_rewards_ += CalculateEarningsForTransactions(
      unreconciled_transactions, 0, to_timestamp);

  ConfirmationsState::Get()->Save();
}

base::Value AdRewards::GetAsDictionary() {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  const double grants_balance = ad_grants_->GetBalance();
  dictionary.SetKey("grants_balance", base::Value(grants_balance));

  base::Value payments = payments_->GetAsList();
  dictionary.SetKey("payments", base::Value(std::move(payments)));

  dictionary.SetKey("unreconciled_estimated_pending_rewards",
                    base::Value(unreconciled_estimated_pending_rewards_));

  return dictionary;
}

bool AdRewards::SetFromDictionary(base::Value* dictionary) {
  DCHECK(dictionary);

  if (!ad_grants_->SetFromDictionary(dictionary) ||
      !payments_->SetFromDictionary(dictionary)) {
    return false;
  }

  const absl::optional<double> unreconciled_estimated_pending_rewards =
      dictionary->FindDoubleKey("unreconciled_estimated_pending_rewards");
  unreconciled_estimated_pending_rewards_ =
      unreconciled_estimated_pending_rewards.value_or(0.0);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool AdRewards::DidReconcile(
    const std::string& json) const {
  const double last_balance = payments_->GetBalance();

  Payments payments;
  if (!payments.SetFromJson(json)) {
    return false;
  }

  if (!payments.DidReconcileBalance(last_balance,
      unreconciled_estimated_pending_rewards_)) {
    return false;
  }

  return true;
}

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

  if (!DidReconcile(url_response.body)) {
    BLOG(0, "Payment balance is not ready");
    OnFailedToReconcileAdRewards();
    return;
  }

  if (!payments_->SetFromJson(url_response.body)) {
    BLOG(0, "Failed to parse payment balance");
    BLOG(6, "Payment balance response body: " << url_response.body);
    OnFailedToReconcileAdRewards();
    return;
  }

  GetAdGrants();
}

void AdRewards::GetAdGrants() {
  BLOG(1, "GetAdGrants");
  BLOG(2, "GET /v1/promotions/ads/grants/summary?paymentId={payment_id}");

  AdGrantsUrlRequestBuilder url_request_builder(wallet_);
  UrlRequestPtr url_request = url_request_builder.Build();
  BLOG(5, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  auto callback =
      std::bind(&AdRewards::OnGetAdGrants, this, std::placeholders::_1);
  AdsClientHelper::Get()->UrlRequest(std::move(url_request), callback);
}

void AdRewards::OnGetAdGrants(const UrlResponse& url_response) {
  BLOG(1, "OnGetAdGrants");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code == net::HTTP_NO_CONTENT) {
    ad_grants_ = std::make_unique<AdGrants>();
    OnDidReconcileAdRewards();
    return;
  }

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to get ad grants");
    OnFailedToReconcileAdRewards();
    return;
  }

  if (!ad_grants_->SetFromJson(url_response.body)) {
    BLOG(0, "Failed to parse ad grants");
    BLOG(6, "Ad grants response body: " << url_response.body);
    OnFailedToReconcileAdRewards();
    return;
  }

  OnDidReconcileAdRewards();
}

void AdRewards::OnDidReconcileAdRewards() {
  is_processing_ = false;

  BLOG(1, "Successfully reconciled ad rewards");

  retry_timer_.Stop();

  unreconciled_estimated_pending_rewards_ = 0.0;
  ConfirmationsState::Get()->Save();

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
