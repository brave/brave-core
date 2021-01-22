/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_REWARDS_AD_REWARDS_H_
#define BAT_ADS_INTERNAL_AD_REWARDS_AD_REWARDS_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/account/ad_rewards/ad_rewards_delegate.h"
#include "bat/ads/internal/backoff_timer.h"
#include "bat/ads/mojom.h"
#include "bat/ads/transaction_info.h"

namespace ads {

class AdGrants;
class Payments;

class AdRewards {
 public:
  AdRewards();

  ~AdRewards();

  void set_delegate(
      AdRewardsDelegate* delegate);

  void MaybeReconcile(
      const WalletInfo& wallet);

  double GetEstimatedPendingRewards() const;

  uint64_t GetNextPaymentDate() const;

  uint64_t GetAdsReceivedThisMonth() const;
  uint64_t GetAdsReceivedForMonth(
      const base::Time& time) const;

  double GetEarningsForThisMonth() const;
  double GetEarningsForMonth(
      const base::Time& time) const;

  double GetUnclearedEarningsForThisMonth() const;

  void SetUnreconciledTransactions(
      const TransactionList& unreconciled_transactions);

  base::Value GetAsDictionary();
  bool SetFromDictionary(
      base::Value* dictionary);

 private:
  bool is_processing_ = false;

  AdRewardsDelegate* delegate_ = nullptr;

  WalletInfo wallet_;

  double unreconciled_estimated_pending_rewards_ = 0.0;

  void Reconcile();

  bool DidReconcile(
      const std::string& json) const;

  void GetPayments();
  void OnGetPayments(
      const UrlResponse& url_response);

  void GetAdGrants();
  void OnGetAdGrants(
      const UrlResponse& url_response);

  void OnDidReconcileAdRewards();

  void OnFailedToReconcileAdRewards();

  BackoffTimer retry_timer_;
  void Retry();
  void OnRetry();

  std::unique_ptr<AdGrants> ad_grants_;
  std::unique_ptr<Payments> payments_;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_REWARDS_AD_REWARDS_H_
