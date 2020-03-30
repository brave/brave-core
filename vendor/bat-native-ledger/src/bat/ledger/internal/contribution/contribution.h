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

#include "base/gtest_prod_util.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {
class Uphold;
}

namespace braveledger_contribution {

class ContributionAC;
class ContributionAnonCard;
class ContributionExternalWallet;
class ContributionMonthly;
class ContributionSKU;
class ContributionTip;
class Unverified;
class Unblinded;

class Contribution {
 public:
  explicit Contribution(bat_ledger::LedgerImpl* ledger);

  ~Contribution();

  void Initialize();

  // Start point for contribution
  // In this step we get balance from the server
  void Start(ledger::ContributionQueuePtr info);

  // Called when timer is triggered
  void OnTimer(uint32_t timer_id);

  // Sets new reconcile timer for monthly contribution in 30 days
  void SetReconcileTimer();

  // Does final stage in contribution
  // Sets reports and contribution info
  void ContributionCompleted(
      const std::string& contribution_id,
      const ledger::RewardsType type,
      const double amount,
      const ledger::Result result);

  void HasSufficientBalance(
    ledger::HasSufficientBalanceToReconcileCallback callback);

  // Fetches recurring tips that will be then used for the contribution.
  // This is called from global timer in impl.
  // Can be also called manually
  void StartMonthlyContribution();

  void SetTimer(uint32_t* timer_id, uint64_t start_timer_in = 0);

  // Resets reconcile stamps
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
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  void RetryUnblinded(
      const std::string& contribution_id,
      ledger::ResultCallback callback);

 private:
  void StartAutoContribute(const ledger::Result result);

  void ContributionCompletedSaved(const ledger::Result result);

  void ProcessContributionQueue();

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

  void DeleteContributionQueue(const uint64_t id);

  void OnDeleteContributionQueue(const ledger::Result result);

  void RetryUnblindedContribution(
      ledger::ContributionInfoPtr contribution,
      ledger::ResultCallback callback);

  void Result(
      const ledger::Result result,
      const std::string& contribution_id);

  void FinishContribution(
      ledger::ContributionInfoPtr contribution,
      const ledger::Result result);

  void SetRetryTimer(
      const std::string& contribution_id,
      const uint64_t& start_timer_in = 0);

  void SetRetryCounter(ledger::ContributionInfoPtr contribution);

  void Retry(
      const ledger::Result result,
      const std::string& contribution_string);

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
  uint32_t last_reconcile_timer_id_;
  std::map<std::string, uint32_t> retry_timers_;
  uint32_t queue_timer_id_;
  bool queue_in_progress_ = false;

  // For testing purposes
  friend class ContributionTest;
  FRIEND_TEST_ALL_PREFIXES(ContributionTest, GetAmountFromVerifiedAuto);
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_H_
