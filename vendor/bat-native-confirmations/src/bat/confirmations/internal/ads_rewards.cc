/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/confirmations/internal/ads_rewards.h"
#include "bat/confirmations/internal/logging.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/get_payment_balance_request.h"
#include "bat/confirmations/internal/get_ad_grants_request.h"

#include "net/http/http_status_code.h"
#include "brave_base/random.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace confirmations {

AdsRewards::AdsRewards(
    ConfirmationsImpl* confirmations,
    ConfirmationsClient* confirmations_client) :
    next_retry_backoff_count_(0),
    retry_timer_id_(0),
    payments_(std::make_unique<Payments>(confirmations, confirmations_client)),
    ad_grants_(std::make_unique<AdGrants>(confirmations, confirmations_client)),
    confirmations_(confirmations),
    confirmations_client_(confirmations_client) {
  BLOG(INFO) << "Initializing ads rewards";
}

AdsRewards::~AdsRewards() {
  BLOG(INFO) << "Deinitializing ads rewards";

  CancelRetry();
}

void AdsRewards::Update(
    const WalletInfo& wallet_info,
    const bool should_refresh) {
  DCHECK(!wallet_info.payment_id.empty());
  DCHECK(!wallet_info.private_key.empty());

  wallet_info_ = WalletInfo(wallet_info);

  Update();

  if (!should_refresh) {
    return;
  }

  BLOG(INFO) << "Fetch ads rewards";
  GetPaymentBalance();
}

base::Value AdsRewards::GetAsDictionary() {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  auto grants_balance = ad_grants_->GetBalance();
  dictionary.SetKey("grants_balance", base::Value(grants_balance));

  auto payments = payments_->GetAsList();
  dictionary.SetKey("payments", base::Value(std::move(payments)));

  return dictionary;
}

bool AdsRewards::SetFromDictionary(base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  auto* ads_rewards_value = dictionary->FindKey("ads_rewards");
  if (!ads_rewards_value || !ads_rewards_value->is_dict()) {
    Update();
    return false;
  }

  base::DictionaryValue* ads_rewards_dictionary;
  if (!ads_rewards_value->GetAsDictionary(&ads_rewards_dictionary)) {
    Update();
    return false;
  }

  auto success = true;

  if (!ad_grants_->SetFromDictionary(ads_rewards_dictionary) ||
      !payments_->SetFromDictionary(ads_rewards_dictionary)) {
    success = false;
  }

  Update();

  return success;
}

bool AdsRewards::OnTimer(const uint32_t timer_id) {
  BLOG(INFO) << "OnTimer: " << std::endl
      << "  timer_id: " << std::to_string(timer_id) << std::endl
      << "  retry_timer_id_: " << std::to_string(retry_timer_id_) << std::endl;

  if (timer_id == retry_timer_id_) {
    Retry();

    return true;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////

void AdsRewards::GetPaymentBalance() {
  BLOG(INFO) << "GetPaymentBalance";

  BLOG(INFO) << "GET /v1/confirmation/payment/{payment_id}";
  GetPaymentBalanceRequest request;

  BLOG(INFO) << "URL Request:";

  auto url = request.BuildUrl(wallet_info_);
  BLOG(INFO) << "  URL: " << url;

  auto method = request.GetMethod();

  auto body = request.BuildBody();
  BLOG(INFO) << "  Body: " << body;

  auto headers = request.BuildHeaders(body, wallet_info_);
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header;
  }

  auto content_type = request.GetContentType();
  BLOG(INFO) << "  Content_type: " << content_type;

  auto callback = std::bind(&AdsRewards::OnGetPaymentBalance,
      this, url, _1, _2, _3);

  confirmations_client_->LoadURL(url, headers, "", "", method, callback);
}

void AdsRewards::OnGetPaymentBalance(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  BLOG(INFO) << "OnGetPaymentBalance";

  BLOG(INFO) << "URL Request Response:";
  BLOG(INFO) << "  URL: " << url;
  BLOG(INFO) << "  Response Status Code: " << response_status_code;
  BLOG(INFO) << "  Response: " << response;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header.first << ": " << header.second;
  }

  if (response_status_code != net::HTTP_OK) {
    BLOG(ERROR) << "Failed to get payment balance";
    OnAdsRewards(FAILED);
    return;
  }

  if (!payments_->SetFromJson(response)) {
    BLOG(ERROR) << "Failed to parse payment balance: " << response;
    OnAdsRewards(FAILED);
    return;
  }

  GetAdGrants();
}

void AdsRewards::GetAdGrants() {
  BLOG(INFO) << "GetAdGrants";

  BLOG(INFO) << "GET /v2/wallet/{payment_id}/grants/ads";
  GetAdGrantsRequest request;

  BLOG(INFO) << "URL Request:";

  auto url = request.BuildUrl(wallet_info_);
  BLOG(INFO) << "  URL: " << url;

  auto method = request.GetMethod();

  auto callback = std::bind(&AdsRewards::OnGetAdGrants, this, url, _1, _2, _3);

  confirmations_client_->LoadURL(url, {}, "", "", method, callback);
}

void AdsRewards::OnGetAdGrants(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  BLOG(INFO) << "OnGetAdGrants";

  BLOG(INFO) << "URL Request Response:";
  BLOG(INFO) << "  URL: " << url;
  BLOG(INFO) << "  Response Status Code: " << response_status_code;
  BLOG(INFO) << "  Response: " << response;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header.first << ": " << header.second;
  }

  if (response_status_code == net::HTTP_NO_CONTENT) {
    ad_grants_ = std::make_unique<AdGrants>(
        confirmations_, confirmations_client_);

    OnAdsRewards(SUCCESS);
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    BLOG(ERROR) << "Failed to get ad grants";
    OnAdsRewards(FAILED);
    return;
  }

  if (!ad_grants_->SetFromJson(response)) {
    BLOG(ERROR) << "Failed to parse ad grants: " << response;
    OnAdsRewards(FAILED);
    return;
  }

  OnAdsRewards(SUCCESS);
}

void AdsRewards::OnAdsRewards(const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to retrieve ads rewards";
    Retry();
    return;
  }

  BLOG(INFO) << "Successfully retrieved ads rewards";

  next_retry_backoff_count_ = 0;

  Update();
}

void AdsRewards::Retry() {
  CancelRetry();

  // Overflow happens only if we have already backed off so many times our
  // expected waiting time is longer than the lifetime of the universe
  auto start_timer_in = 1 * base::Time::kSecondsPerMinute;
  start_timer_in <<= next_retry_backoff_count_++;

  auto rand_delay = brave_base::random::Geometric(start_timer_in);

  confirmations_client_->SetTimer(rand_delay, &retry_timer_id_);
  if (retry_timer_id_ == 0) {
    BLOG(ERROR) << "Failed to retry due to an invalid timer";
    return;
  }

  BLOG(INFO) << "Start retrying in " << rand_delay << " seconds";
}

void AdsRewards::CancelRetry() {
  if (!IsRetrying()) {
    return;
  }

  BLOG(INFO) << "Cancelled retry";

  confirmations_client_->KillTimer(retry_timer_id_);
  retry_timer_id_ = 0;
}

bool AdsRewards::IsRetrying() const {
  if (retry_timer_id_ == 0) {
    return false;
  }

  return true;
}

void AdsRewards::Update() {
  auto estimated_pending_rewards = CalculateEstimatedPendingRewards();

  auto now = base::Time::Now();
  auto next_payment_date = payments_->CalculateNextPaymentDate(now,
      confirmations_->GetNextTokenRedemptionDateInSeconds());
  uint64_t next_payment_date_in_seconds = next_payment_date.ToDoubleT();

  auto ad_notifications_received_this_month =
      payments_->GetTransactionCountForMonth(now);

  confirmations_->UpdateAdsRewards(estimated_pending_rewards,
      next_payment_date_in_seconds, ad_notifications_received_this_month);
}

double AdsRewards::CalculateEstimatedPendingRewards() const {
  auto estimated_pending_rewards = payments_->GetBalance();

  estimated_pending_rewards -= ad_grants_->GetBalance();
  if (estimated_pending_rewards < 0.0) {
    estimated_pending_rewards = 0.0;
  }

  return estimated_pending_rewards;
}

}  // namespace confirmations
