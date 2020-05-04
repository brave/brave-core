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
#include "bat/confirmations/internal/static_values.h"
#include "bat/confirmations/internal/time_util.h"

#include "net/http/http_status_code.h"
#include "brave_base/random.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace confirmations {

AdsRewards::AdsRewards(
    ConfirmationsImpl* confirmations)
    : payments_(std::make_unique<Payments>()),
      ad_grants_(std::make_unique<AdGrants>()),
      confirmations_(confirmations) {
}

AdsRewards::~AdsRewards() = default;

void AdsRewards::Update(
    const WalletInfo& wallet_info,
    const bool should_refresh) {
  Update();

  if (!should_refresh) {
    return;
  }

  if (retry_timer_.IsRunning()) {
    return;
  }

  wallet_info_ = wallet_info;
  if (!wallet_info_.IsValid()) {
    BLOG(0, "Failed to refresh ads rewards due to invalid wallet");
    return;
  }

  BLOG(1, "Refresh ads rewards");
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

///////////////////////////////////////////////////////////////////////////////

void AdsRewards::GetPaymentBalance() {
  BLOG(1, "GetPaymentBalance");
  BLOG(2, "GET /v1/confirmation/payment/{payment_id}");

  GetPaymentBalanceRequest request;
  auto url = request.BuildUrl(wallet_info_);
  auto method = request.GetMethod();
  auto body = request.BuildBody();
  auto headers = request.BuildHeaders(body, wallet_info_);
  auto content_type = request.GetContentType();

  auto callback = std::bind(&AdsRewards::OnGetPaymentBalance, this, _1);

  BLOG(5, UrlRequestToString(url, headers, "", "", method));
  confirmations_->get_client()->LoadURL(url, headers, "", "", method, callback);
}

void AdsRewards::OnGetPaymentBalance(
    const UrlResponse& url_response) {
  BLOG(1, "OnGetPaymentBalance");

  BLOG(6, UrlResponseToString(url_response));

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to get payment balance");
    OnAdsRewards(FAILED);
    return;
  }

  if (!payments_->SetFromJson(url_response.body)) {
    BLOG(0, "Failed to parse payment balance: " << url_response.body);
    OnAdsRewards(FAILED);
    return;
  }

  GetAdGrants();
}

void AdsRewards::GetAdGrants() {
  BLOG(1, "GetAdGrants");
  BLOG(2, "GET /v1/promotions/ads/grants/summary?paymentId={payment_id}");

  GetAdGrantsRequest request;
  auto url = request.BuildUrl(wallet_info_);
  auto method = request.GetMethod();

  auto callback = std::bind(&AdsRewards::OnGetAdGrants, this, _1);

  BLOG(5, UrlRequestToString(url, {}, "", "", method));
  confirmations_->get_client()->LoadURL(url, {}, "", "", method, callback);
}

void AdsRewards::OnGetAdGrants(
    const UrlResponse& url_response) {
  BLOG(1, "OnGetAdGrants");

  BLOG(6, UrlResponseToString(url_response));

  if (url_response.status_code == net::HTTP_NO_CONTENT) {
    ad_grants_ = std::make_unique<AdGrants>();

    OnAdsRewards(SUCCESS);
    return;
  }

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to get ad grants");
    OnAdsRewards(FAILED);
    return;
  }

  if (!ad_grants_->SetFromJson(url_response.body)) {
    BLOG(0, "Failed to parse ad grants: " << url_response.body);
    OnAdsRewards(FAILED);
    return;
  }

  OnAdsRewards(SUCCESS);
}

void AdsRewards::OnAdsRewards(const Result result) {
  if (result != SUCCESS) {
    BLOG(1, "Failed to get ads rewards");

    const base::Time time = retry_timer_.StartWithBackoff(
        kRetryAdsRewardsAfterSeconds, base::BindOnce(&AdsRewards::OnRetry,
            base::Unretained(this)));

    BLOG(1, "Retry getting ad grants " << FriendlyDateAndTime(time));

    return;
  }

  BLOG(1, "Successfully retrieved ads rewards");

  retry_timer_.Stop();

  Update();
}

void AdsRewards::OnRetry() {
  BLOG(1, "Retrying getting ads rewards");

  GetPaymentBalance();
}

void AdsRewards::Update() {
  auto estimated_pending_rewards = CalculateEstimatedPendingRewards();

  auto now = base::Time::Now();
  auto next_payment_date = payments_->CalculateNextPaymentDate(now,
      confirmations_->GetNextTokenRedemptionDateInSeconds());
  uint64_t next_payment_date_in_seconds =
      static_cast<uint64_t>(next_payment_date.ToDoubleT());

  confirmations_->UpdateAdsRewards(estimated_pending_rewards,
      next_payment_date_in_seconds);
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
