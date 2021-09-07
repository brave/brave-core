/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_AD_REWARDS_AD_REWARDS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_AD_REWARDS_AD_REWARDS_H_

#include <cstdint>
#include <memory>

#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/backoff_timer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "bat/ads/transaction_info.h"

namespace ads {

class AdRewardsDelegate;
class Payments;

class AdRewards {
 public:
  AdRewards();

  ~AdRewards();

  void set_delegate(AdRewardsDelegate* delegate);

  void MaybeReconcile(const WalletInfo& wallet);

  uint64_t GetNextPaymentDate() const;

  uint64_t GetAdsReceivedThisMonth() const;
  uint64_t GetAdsReceivedForMonth(const base::Time& time) const;

  double GetEarningsForThisMonth() const;
  double GetEarningsForMonth(const base::Time& time) const;

  double GetUnclearedEarningsForThisMonth() const;

  void AppendUnreconciledTransactions(const TransactionList& transactions);

  base::Value GetAsDictionary();
  bool SetFromDictionary(base::Value* dictionary);

  void Reset();

 private:
  bool is_processing_ = false;

  AdRewardsDelegate* delegate_ = nullptr;

  WalletInfo wallet_;

  double unreconciled_transactions_ = 0.0;
  void ClearUnreconciledTransactions();

  void Reconcile();

  void GetPayments();
  void OnGetPayments(const mojom::UrlResponse& url_response);

  void OnDidReconcileAdRewards();
  void OnFailedToReconcileAdRewards();

  BackoffTimer retry_timer_;
  void Retry();
  void OnRetry();

  std::unique_ptr<Payments> payments_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_AD_REWARDS_AD_REWARDS_H_
