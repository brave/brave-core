/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SERVER_GET_AD_REWARDS_GET_AD_REWARDS_H_
#define BAT_ADS_INTERNAL_SERVER_GET_AD_REWARDS_GET_AD_REWARDS_H_

#include <stdint.h>

#include <memory>

#include "base/values.h"
#include "bat/ads/internal/backoff_timer.h"
#include "bat/ads/internal/server/ad_rewards/ad_grants/ad_grants.h"
#include "bat/ads/internal/server/ad_rewards/payments/payments.h"
#include "bat/ads/internal/wallet/wallet_info.h"
#include "bat/ads/mojom.h"
#include "bat/ads/result.h"
#include "bat/ads/transaction_info.h"

namespace ads {

class AdsImpl;

class AdRewards {
 public:
  AdRewards(
      AdsImpl* ads);

  ~AdRewards();

  void Update(
      const WalletInfo& wallet,
      const bool should_reconcile);

  double GetEstimatedPendingRewards() const;
  uint64_t GetNextPaymentDateInSeconds() const;
  uint64_t GetAdNotificationsReceivedThisMonth() const;

  double CalculateEstimatedPendingRewardsForTransactions(
      const TransactionList& transactions) const;

  void SetUnreconciledTransactions(
      const TransactionList& unreconciled_transactions);

  base::Value GetAsDictionary();
  bool SetFromDictionary(
      base::Value* dictionary);

 private:
  WalletInfo wallet_;

  double unreconciled_estimated_pending_rewards_ = 0.0;

  void GetPayments();
  void OnGetPayments(
      const UrlResponse& url_response);

  void GetAdGrants();
  void OnGetAdGrants(
      const UrlResponse& url_response);

  void OnAdRewards(
      const Result result);

  BackoffTimer retry_timer_;
  void Retry();

  uint64_t CalculateAdNotificationsReceivedThisMonthForTransactions(
      const TransactionList& transactions) const;

  AdsImpl* ads_;  // NOT OWNED

  std::unique_ptr<AdGrants> ad_grants_;
  std::unique_ptr<Payments> payments_;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SERVER_GET_AD_REWARDS_GET_AD_REWARDS_H_
