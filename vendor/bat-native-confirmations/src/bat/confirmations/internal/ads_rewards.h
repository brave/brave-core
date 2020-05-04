/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_ADS_REWARDS_H_
#define BAT_CONFIRMATIONS_INTERNAL_ADS_REWARDS_H_

#include <string>
#include <map>
#include <memory>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/wallet_info.h"
#include "bat/confirmations/internal/payments.h"
#include "bat/confirmations/internal/ad_grants.h"
#include "bat/confirmations/internal/retry_timer.h"

#include "base/values.h"

namespace confirmations {

class ConfirmationsImpl;

class AdsRewards {
 public:
  AdsRewards(
      ConfirmationsImpl* confirmations);

  ~AdsRewards();

  void Update(const WalletInfo& wallet_info, const bool should_refresh);

  base::Value GetAsDictionary();
  bool SetFromDictionary(base::DictionaryValue* dictionary);

 private:
  WalletInfo wallet_info_;

  void GetPaymentBalance();
  void OnGetPaymentBalance(
      const UrlResponse& url_response);

  void GetAdGrants();
  void OnGetAdGrants(
      const UrlResponse& url_response);

  void OnAdsRewards(
      const Result result);

  RetryTimer retry_timer_;
  void OnRetry();

  void Update();
  double CalculateEstimatedPendingRewards() const;

  std::unique_ptr<Payments> payments_;
  std::unique_ptr<AdGrants> ad_grants_;

  ConfirmationsImpl* confirmations_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_ADS_REWARDS_H_
