/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/ad_rewards/ad_rewards.h"

#include <functional>
#include <utility>

#include "net/http/http_status_code.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/server/ad_rewards/ad_grants/ad_grants_url_request_builder.h"
#include "bat/ads/internal/server/ad_rewards/payments/payments_url_request_builder.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

using std::placeholders::_1;

namespace {
const uint64_t kRetryAfterSeconds = 1 * base::Time::kSecondsPerMinute;
}  // namespace

AdRewards::AdRewards(
    AdsImpl* ads)
    : ads_(ads),
      ad_grants_(std::make_unique<AdGrants>()),
      payments_(std::make_unique<Payments>()) {
  DCHECK(ads_);
}

AdRewards::~AdRewards() = default;

void AdRewards::Update(
    const WalletInfo& wallet,
    const bool should_reconcile) {
  if (!should_reconcile) {
    return;
  }

  if (retry_timer_.IsRunning()) {
    return;
  }

  wallet_ = wallet;
  if (!wallet_.IsValid()) {
    BLOG(0, "Failed to get ad rewards due to invalid wallet");
    return;
  }

  BLOG(1, "Reconcile ad rewards with server");
  GetPayments();
}

double AdRewards::GetEstimatedPendingRewards() const {
  double estimated_pending_rewards = payments_->GetBalance();

  estimated_pending_rewards -= ad_grants_->GetBalance();

  const TransactionList unredeemed_transactions =
      ads_->GetUnredeemedTransactions();
  const double unredeemed_estimated_pending_rewards =
      CalculateEstimatedPendingRewardsForTransactions(unredeemed_transactions);
  estimated_pending_rewards += unredeemed_estimated_pending_rewards;

  estimated_pending_rewards += unreconciled_estimated_pending_rewards_;

  if (estimated_pending_rewards < 0.0) {
    estimated_pending_rewards = 0.0;
  }

  return estimated_pending_rewards;
}

uint64_t AdRewards::GetNextPaymentDateInSeconds() const {
  const base::Time now = base::Time::Now();

  const base::Time next_token_redemption_date =
      ads_->get_confirmations()->get_next_token_redemption_date();

  const base::Time next_payment_date =
      payments_->CalculateNextPaymentDate(now, next_token_redemption_date);

  return static_cast<uint64_t>(next_payment_date.ToDoubleT());
}

uint64_t AdRewards::GetAdNotificationsReceivedThisMonth() const {
  const TransactionList transactions =
      ads_->get_confirmations()->get_transactions();
  return CalculateAdNotificationsReceivedThisMonthForTransactions(transactions);
}

void AdRewards::SetUnreconciledTransactions(
    const TransactionList& unreconciled_transactions) {
  unreconciled_estimated_pending_rewards_ =
      CalculateEstimatedPendingRewardsForTransactions(
          unreconciled_transactions);

  ads_->get_confirmations()->Save();
}

double AdRewards::CalculateEstimatedPendingRewardsForTransactions(
    const TransactionList& transactions) const {
  double estimated_pending_rewards = 0.0;

  for (const auto& transaction : transactions) {
    estimated_pending_rewards += transaction.estimated_redemption_value;
  }

  return estimated_pending_rewards;
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

bool AdRewards::SetFromDictionary(
    base::Value* dictionary) {
  DCHECK(dictionary);

  base::Value* ad_rewards = dictionary->FindDictKey("ads_rewards");
  if (!ad_rewards) {
    return false;
  }

  base::DictionaryValue* ads_rewards_dictionary;
  if (!ad_rewards->GetAsDictionary(&ads_rewards_dictionary)) {
    return false;
  }

  if (!ad_grants_->SetFromDictionary(ads_rewards_dictionary) ||
      !payments_->SetFromDictionary(ads_rewards_dictionary)) {
    return false;
  }

  const base::Optional<double> unreconciled_estimated_pending_rewards =
      ads_rewards_dictionary->FindDoubleKey(
          "unreconciled_estimated_pending_rewards");
  unreconciled_estimated_pending_rewards_ =
      unreconciled_estimated_pending_rewards.value_or(0.0);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

void AdRewards::GetPayments() {
  BLOG(1, "GetPayments");
  BLOG(2, "GET /v1/confirmation/payment/{payment_id}");

  PaymentsUrlRequestBuilder url_request_builder(wallet_);
  UrlRequestPtr url_request = url_request_builder.Build();
  BLOG(5, UrlRequestToString(url_request));

  auto callback = std::bind(&AdRewards::OnGetPayments, this, _1);
  ads_->get_ads_client()->UrlRequest(std::move(url_request), callback);
}

void AdRewards::OnGetPayments(
    const UrlResponse& url_response) {
  BLOG(1, "OnGetPayments");

  BLOG(6, UrlResponseToString(url_response));

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to get payment balance");
    OnAdRewards(FAILED);
    return;
  }

  if (!payments_->SetFromJson(url_response.body)) {
    BLOG(0, "Failed to parse payment balance: " << url_response.body);
    OnAdRewards(FAILED);
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

  auto callback = std::bind(&AdRewards::OnGetAdGrants, this, _1);
  ads_->get_ads_client()->UrlRequest(std::move(url_request), callback);
}

void AdRewards::OnGetAdGrants(
    const UrlResponse& url_response) {
  BLOG(1, "OnGetGrants");

  BLOG(6, UrlResponseToString(url_response));

  if (url_response.status_code == net::HTTP_NO_CONTENT) {
    ad_grants_ = std::make_unique<AdGrants>();
    OnAdRewards(SUCCESS);
    return;
  }

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to get ad grants");
    OnAdRewards(FAILED);
    return;
  }

  if (!ad_grants_->SetFromJson(url_response.body)) {
    BLOG(0, "Failed to parse ad grants: " << url_response.body);
    OnAdRewards(FAILED);
    return;
  }

  OnAdRewards(SUCCESS);
}

void AdRewards::OnAdRewards(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(1, "Failed to get ad rewards");

    const base::Time time = retry_timer_.StartWithPrivacy(
        base::TimeDelta::FromSeconds(kRetryAfterSeconds),
            base::BindOnce(&AdRewards::Retry, base::Unretained(this)));

    BLOG(1, "Retry getting ad grants " << FriendlyDateAndTime(time));

    return;
  }

  BLOG(1, "Successfully retrieved ad rewards");

  retry_timer_.Stop();

  unreconciled_estimated_pending_rewards_ = 0.0;
  ads_->get_confirmations()->Save();

  ads_->get_ads_client()->OnAdRewardsChanged();
}

void AdRewards::Retry() {
  BLOG(1, "Retrying getting ad rewards");

  GetPayments();
}

uint64_t AdRewards::CalculateAdNotificationsReceivedThisMonthForTransactions(
    const TransactionList& transactions) const {
  uint64_t ad_notifications_received_this_month = 0;

  auto now = base::Time::Now();
  base::Time::Exploded now_exploded;
  now.UTCExplode(&now_exploded);

  for (const auto& transaction : transactions) {
    if (transaction.timestamp_in_seconds == 0) {
      // Workaround for Windows crash when passing 0 to UTCExplode
      continue;
    }

    auto transaction_timestamp =
        base::Time::FromDoubleT(transaction.timestamp_in_seconds);

    base::Time::Exploded transaction_timestamp_exploded;
    transaction_timestamp.UTCExplode(&transaction_timestamp_exploded);

    if (transaction_timestamp_exploded.year == now_exploded.year &&
        transaction_timestamp_exploded.month == now_exploded.month &&
        transaction.estimated_redemption_value > 0.0 &&
        ConfirmationType(transaction.confirmation_type) ==
            ConfirmationType::kViewed) {
      ad_notifications_received_this_month++;
    }
  }

  return ad_notifications_received_this_month;
}

}  // namespace ads
