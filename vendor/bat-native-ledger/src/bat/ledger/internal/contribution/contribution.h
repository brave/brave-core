/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/timer/timer.h"
#include "bat/ledger/internal/contribution/contribution_ac.h"
#include "bat/ledger/internal/contribution/contribution_anon_card.h"
#include "bat/ledger/internal/contribution/contribution_external_wallet.h"
#include "bat/ledger/internal/contribution/contribution_monthly.h"
#include "bat/ledger/internal/contribution/contribution_sku.h"
#include "bat/ledger/internal/contribution/contribution_tip.h"
#include "bat/ledger/internal/contribution/contribution_unblinded.h"
#include "bat/ledger/internal/contribution/unverified.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {
class Uphold;
}

namespace braveledger_contribution {

class Contribution {
 public:
  explicit Contribution(bat_ledger::LedgerImpl* ledger);

  ~Contribution();

  void Initialize();

  void ProcessContributionQueue();

  // Sets new reconcile timer for monthly contribution in 30 days
  void SetReconcileTimer();

  // Does final stage in contribution
  // Sets reports and contribution info
  void ContributionCompleted(
      const ledger::Result result,
      ledger::ContributionInfoPtr contribution);

  void HasSufficientBalance(
    ledger::HasSufficientBalanceToReconcileCallback callback);

  // Fetches recurring tips that will be then used for the contribution.
  // This is called from global timer in impl.
  // Can be also called manually
  void StartMonthlyContribution();

  // Reset reconcile stamps
  void ResetReconcileStamp();

  void ContributeUnverifiedPublishers();

  void OneTimeTip(
      const std::string& publisher_key,
      const double amount,
      ledger::ResultCallback callback);

  void CheckContributionQueue();

  void TransferFunds(
      const ledger::SKUTransaction& transaction,
      const std::string& destination,
      ledger::ExternalWalletPtr wallet,
      ledger::TransactionCallback callback);

  void SKUAutoContribution(
      const std::string& contribution_id,
      ledger::ExternalWalletPtr wallet,
      ledger::ResultCallback callback);

  void StartUnblinded(
      const std::vector<ledger::CredsBatchType>& types,
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void RetryUnblinded(
      const std::vector<ledger::CredsBatchType>& types,
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void GetRecurringTips(ledger::PublisherInfoListCallback callback);

 private:
  // Start point for contribution
  // In this step we get balance from the server
  void Start(ledger::ContributionQueuePtr info);

  void StartAutoContribute(
      const ledger::Result result,
      const uint64_t reconcile_stamp);

  void ContributionCompletedSaved(
      const ledger::Result result,
      const std::string& contribution_id);

  void OnProcessContributionQueue(ledger::ContributionQueuePtr info);

  void CheckNotCompletedContributions();

  void NotCompletedContributions(ledger::ContributionInfoList list);

  void OnBalance(
      const std::string& contribution_queue,
      const ledger::Result result,
      ledger::BalancePtr info);

  void CreateNewEntry(
      const std::string& wallet_type,
      ledger::BalancePtr balance,
      ledger::ContributionQueuePtr queue);

  void OnEntrySaved(
      const ledger::Result result,
      const std::string& contribution_id,
      const std::string& wallet_type,
      const ledger::Balance& balance,
      const std::string& queue_string);

  void OnQueueSaved(
      const ledger::Result result,
      const std::string& wallet_type,
      const ledger::Balance& balance,
      const std::string& queue_string);

  void Process(
      ledger::ContributionQueuePtr queue,
      ledger::BalancePtr balance);

  void MarkContributionQueueAsComplete(const std::string& id);

  void OnMarkContributionQueueAsComplete(const ledger::Result result);

  void RetryUnblindedContribution(
      ledger::ContributionInfoPtr contribution,
      const std::vector<ledger::CredsBatchType>& types,
      ledger::ResultCallback callback);

  void Result(
      const ledger::Result result,
      const std::string& contribution_id);

  void OnResult(
      ledger::ContributionInfoPtr contribution,
      const ledger::Result result);

  void SetRetryTimer(
      const std::string& contribution_id,
      base::TimeDelta delay);

  void OnRetryTimerElapsed(const std::string& contribution_id);

  void SetRetryCounter(ledger::ContributionInfoPtr contribution);

  void Retry(
      const ledger::Result result,
      const std::string& contribution_string);

  void OnMarkUnblindedTokensAsSpendable(
      const ledger::Result result,
      const std::string& contribution_id);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<Unverified> unverified_;
  std::unique_ptr<Unblinded> unblinded_;
  std::unique_ptr<ContributionSKU> sku_;
  std::unique_ptr<braveledger_uphold::Uphold> uphold_;
  std::unique_ptr<ContributionMonthly> monthly_;
  std::unique_ptr<ContributionAC> ac_;
  std::unique_ptr<ContributionTip> tip_;
  std::unique_ptr<ContributionExternalWallet> external_wallet_;
  std::unique_ptr<ContributionAnonCard> anon_card_;
  base::OneShotTimer last_reconcile_timer_;
  std::map<std::string, base::OneShotTimer> retry_timers_;
  base::OneShotTimer queue_timer_;
  bool queue_in_progress_ = false;
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_H_
