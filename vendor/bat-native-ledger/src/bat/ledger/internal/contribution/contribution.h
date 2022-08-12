/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/timer/timer.h"
#include "bat/ledger/internal/contribution/contribution_ac.h"
#include "bat/ledger/internal/contribution/contribution_external_wallet.h"
#include "bat/ledger/internal/contribution/contribution_monthly.h"
#include "bat/ledger/internal/contribution/contribution_sku.h"
#include "bat/ledger/internal/contribution/contribution_tip.h"
#include "bat/ledger/internal/contribution/contribution_unblinded.h"
#include "bat/ledger/internal/contribution/unverified.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace contribution {

class Contribution {
 public:
  explicit Contribution(LedgerImpl* ledger);

  ~Contribution();

  void Initialize();

  void ProcessContributionQueue();

  // Sets new reconcile timer for monthly contribution in 30 days
  void SetReconcileTimer();

  // Does final stage in contribution
  // Sets reports and contribution info
  void ContributionCompleted(
      const type::Result result,
      type::ContributionInfoPtr contribution);

  void HasSufficientBalance(
    ledger::HasSufficientBalanceToReconcileCallback callback);

  // Fetches recurring tips that will be then used for the contribution.
  // This is called from global timer in impl.
  // Can be also called manually
  void StartMonthlyContribution();

  // Reset reconcile stamps
  void ResetReconcileStamp();

  void ContributeUnverifiedPublishers();

  void OneTimeTip(const std::string& publisher_key,
                  double amount,
                  ledger::LegacyResultCallback callback);

  void CheckContributionQueue();

  void TransferFunds(
      const type::SKUTransaction& transaction,
      const std::string& destination,
      const std::string& wallet_type,
      client::TransactionCallback callback);

  void SKUAutoContribution(const std::string& contribution_id,
                           const std::string& wallet_type,
                           ledger::LegacyResultCallback callback);

  void StartUnblinded(const std::vector<type::CredsBatchType>& types,
                      const std::string& contribution_id,
                      ledger::LegacyResultCallback callback);

  void RetryUnblinded(const std::vector<type::CredsBatchType>& types,
                      const std::string& contribution_id,
                      ledger::LegacyResultCallback callback);

  void GetRecurringTips(ledger::PublisherInfoListCallback callback);

 private:
  // Start point for contribution
  // In this step we get balance from the server
  void Start(type::ContributionQueuePtr info);

  void StartAutoContribute(
      const type::Result result,
      const uint64_t reconcile_stamp);

  void ContributionCompletedSaved(
      const type::Result result,
      const std::string& contribution_id);

  void OnProcessContributionQueue(type::ContributionQueuePtr info);

  void CheckNotCompletedContributions();

  void NotCompletedContributions(type::ContributionInfoList list);

  void OnBalance(type::ContributionQueuePtr queue,
                 const type::Result result,
                 type::BalancePtr info);

  void CreateNewEntry(
      const std::string& wallet_type,
      type::BalancePtr balance,
      type::ContributionQueuePtr queue);

  void OnEntrySaved(
      const type::Result result,
      const std::string& contribution_id,
      const std::string& wallet_type,
      const type::Balance& balance,
      std::shared_ptr<type::ContributionQueuePtr> shared_queue);

  void OnQueueSaved(
      const type::Result result,
      const std::string& wallet_type,
      const type::Balance& balance,
      std::shared_ptr<type::ContributionQueuePtr> shared_queue);

  void Process(
      type::ContributionQueuePtr queue,
      type::BalancePtr balance);

  void MarkContributionQueueAsComplete(const std::string& id);

  void OnMarkContributionQueueAsComplete(const type::Result result);

  void RetryUnblindedContribution(
      type::ContributionInfoPtr contribution,
      const std::vector<type::CredsBatchType>& types,
      ledger::LegacyResultCallback callback);

  void Result(
      const type::Result result,
      const std::string& contribution_id);

  void OnResult(
      type::ContributionInfoPtr contribution,
      const type::Result result);

  void SetRetryTimer(
      const std::string& contribution_id,
      base::TimeDelta delay);

  void OnRetryTimerElapsed(const std::string& contribution_id);

  void SetRetryCounter(type::ContributionInfoPtr contribution);

  void Retry(
      const type::Result result,
      std::shared_ptr<type::ContributionInfoPtr> shared_contribution);

  void OnMarkUnblindedTokensAsSpendable(
      const type::Result result,
      const std::string& contribution_id);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<Unverified> unverified_;
  std::unique_ptr<Unblinded> unblinded_;
  std::unique_ptr<ContributionSKU> sku_;
  std::unique_ptr<ContributionMonthly> monthly_;
  std::unique_ptr<ContributionAC> ac_;
  std::unique_ptr<ContributionTip> tip_;
  std::unique_ptr<ContributionExternalWallet> external_wallet_;
  base::OneShotTimer last_reconcile_timer_;
  std::map<std::string, base::OneShotTimer> retry_timers_;
  base::OneShotTimer queue_timer_;
  bool queue_in_progress_ = false;
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_CONTRIBUTION_H_
