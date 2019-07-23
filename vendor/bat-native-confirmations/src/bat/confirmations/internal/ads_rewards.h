/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_ADS_REWARDS_H_
#define BAT_CONFIRMATIONS_INTERNAL_ADS_REWARDS_H_

#include <stdint.h>
#include <string>
#include <map>
#include <memory>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/wallet_info.h"
#include "bat/confirmations/internal/payments.h"
#include "bat/confirmations/internal/ad_grants.h"

#include "base/values.h"

namespace confirmations {

class ConfirmationsImpl;

class AdsRewards {
 public:
  AdsRewards(
      ConfirmationsImpl* confirmations,
      ConfirmationsClient* confirmations_client);

  ~AdsRewards();

  void Update(const WalletInfo& wallet_info, const bool should_refresh);

  base::Value GetAsDictionary();
  bool SetFromDictionary(base::DictionaryValue* dictionary);

  bool OnTimer(const uint32_t timer_id);

 private:
  WalletInfo wallet_info_;

  void GetPaymentBalance();
  void OnGetPaymentBalance(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void GetAdGrants();
  void OnGetAdGrants(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnAdsRewards(
      const Result result);

  uint64_t next_retry_backoff_count_;
  uint32_t retry_timer_id_;
  void Retry();
  void CancelRetry();
  bool IsRetrying() const;

  void Update();
  double CalculateEstimatedPendingRewards() const;

  std::unique_ptr<Payments> payments_;
  std::unique_ptr<AdGrants> ad_grants_;

  ConfirmationsImpl* confirmations_;  // NOT OWNED
  ConfirmationsClient* confirmations_client_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_ADS_REWARDS_H_
