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
#include "bat/ledger/internal/properties/current_reconcile_properties.h"

// Contribution has two big phases. PHASE 1 is starting the contribution,
// getting surveyors and transferring BAT from the wallet.
// PHASE 2 uses surveyors from the phase 1 and client generates votes/ballots
// and send them to the server so that server knows to
// which publisher sends the money.

// For every phase we are doing retries, so that we try our best to process
// contribution successfully. In Phase 1 we notify users about the failure after
// we do the whole interval of retries. In Phase 2 we have shorter interval
// but we will try indefinitely, because we just need to send data to the server
// and we don't need anything from the server.

// Re-try interval for Phase 1:
// 1 hour
// 6 hours
// 12 hours
// 24 hours
// 48 hours
// stop contribution and report error to the user

// Re-try interval for Phase 2:
// 1 hour
// 6 hours
// 24 hours
// repeat 24 hours interval


// Contribution process

// PHASE 0
// 1. InitReconcile
// 2. ProcessReconcile

// PHASE 1 (reconcile)
// 1. Start (Reconcile)
// 3. ReconcileCallback
// 4. CurrentReconcile
// 5. CurrentReconcileCallback
// 6. ReconcilePayload
// 7. ReconcilePayloadCallback
// 8. RegisterViewing
// 9. RegisterViewingCallback
// 10. ViewingCredentials
// 11. ViewingCredentialsCallback
// 12. Complete

// PHASE 2 (voting)
// 1. Start (GetReconcileWinners)
// 2. VotePublishers
// 3. VotePublisher
// 4. PrepareBallots
// 5. PrepareBatch
// 6. PrepareBatchCallback
// 7. ProofBatch
// 8. ProofBatchCallback
// 9. SetTimer
// 10. PrepareVoteBatch
// 12. SetTimer
// 12. VoteBatch
// 13. VoteBatchCallback
// 14. SetTimer - we set timer until the whole batch is processed

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {
class Uphold;
}

namespace braveledger_contribution {

class PhaseOne;
class PhaseTwo;
class Unverified;
class Unblinded;

static const uint64_t phase_one_timers[] = {
    1 * 60 * 60,  // 1h
    2 * 60 * 60,  // 2h
    12 * 60 * 60,  // 12h
    24 * 60 * 60,  // 24h
    48 * 60 * 60};  // 48h

static const uint64_t phase_two_timers[] = {
    1 * 60 * 60,  // 1h
    6 * 60 * 60,  // 6h
    24 * 60 * 60};  // 24h

static const uint64_t phase_one_debug_timers[] = {
    0.5 * 60,  // 30sec
    1 * 60,  //  1min
    2 * 60,  //  2min
    3 * 60,  // 3min
    4 * 60};  // 4min

static const uint64_t phase_two_debug_timers[] = {
    1 * 60,  // 1min
    2 * 60,  //  2min
    3 * 60};  // 3min

class Contribution {
 public:
  explicit Contribution(bat_ledger::LedgerImpl* ledger);

  ~Contribution();

  void Initialize();

  // Initial point for contribution
  // In this step we get balance from the server
  void InitReconcile(ledger::ContributionQueuePtr info);

  // Called when timer is triggered
  void OnTimer(uint32_t timer_id);

  // Sets new reconcile timer for monthly contribution in 30 days
  void SetReconcileTimer();

  // Does final stage in contribution
  // Sets reports and contribution info
  // DEPRECATED
  void ReconcileSuccess(
      const std::string& viewing_id,
      const double amount,
      const bool delete_reconcile);

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

  bool ShouldStartAutoContribute();

  void SetTimer(uint32_t* timer_id, uint64_t start_timer_in = 0);

  // DEPRECATED
  void AddRetry(
    ledger::ContributionRetry step,
    const std::string& viewing_id,
    ledger::CurrentReconcileProperties reconcile = {});

  void UpdateContributionStepAndCount(
      const std::string& contribution_id,
      const ledger::ContributionStep step,
      const int32_t retry_count);

  // Resets reconcile stamps
  void ResetReconcileStamp();

  // Triggers contribution process for auto contribute table
  void StartAutoContribute(uint64_t reconcile_stamp);

  void ContributeUnverifiedPublishers();

  void StartPhaseTwo(const std::string& viewing_id);

  void DoDirectTip(
      const std::string& publisher_key,
      double amount,
      const std::string& currency,
      ledger::DoDirectTipCallback callback);

 private:
  void CheckContributionQueue();

  void ProcessContributionQueue();

  void OnProcessContributionQueue(ledger::ContributionQueuePtr info);

  // RECURRING TIPS: from the list gets only verified publishers and
  // save unverified to the db
  ledger::PublisherInfoList GetVerifiedListRecurring(
      const ledger::PublisherInfoList& all);

  void PrepareACList(ledger::PublisherInfoList list,
                     uint32_t next_record);

  void StartRecurringTips(ledger::ResultCallback callback);

  void PrepareRecurringList(
      ledger::PublisherInfoList list,
      uint32_t next_record,
      ledger::ResultCallback callback);

  void OnStartRecurringTips(const ledger::Result result);

  void OnBalanceForReconcile(
      const std::string& contribution_queue,
      const ledger::Result result,
      ledger::BalancePtr info);

  // DEPRECATED
  uint64_t GetRetryTimer(ledger::ContributionRetry step,
                         const std::string& viewing_id,
                         ledger::CurrentReconcileProperties* reconcile);

  // DEPRECATED
  int GetRetryPhase(ledger::ContributionRetry step);

  // DEPRECATED
  void DoRetry(const std::string& viewing_id);

  void CheckStep(const std::string& contribution_id);

  void OnHasSufficientBalance(
      const ledger::PublisherInfoList& publisher_list,
      const uint32_t record,
      const double balance,
      ledger::HasSufficientBalanceToReconcileCallback callback);

  static double GetTotalFromRecurringVerified(
      const ledger::PublisherInfoList& publisher_list);

  void OnSufficientBalanceWallet(
      ledger::Result result,
      ledger::BalancePtr properties,
      ledger::HasSufficientBalanceToReconcileCallback callback);

  void SavePendingContribution(
      const std::string& publisher_key,
      double amount,
      const ledger::RewardsType type,
      ledger::SavePendingContributionCallback callback);

  void OnDoDirectTipServerPublisher(
    ledger::ServerPublisherInfoPtr server_info,
    const std::string& publisher_key,
    double amount,
    const std::string& currency,
    ledger::DoDirectTipCallback callback);

  bool HaveReconcileEnoughFunds(
      ledger::ContributionQueuePtr contribution,
      double* fee,
      const double balance);

  bool ProcessReconcileUnblindedTokens(
      ledger::BalancePtr info,
      ledger::RewardsType type,
      double* fee,
      ledger::ReconcileDirections directions,
      ledger::ReconcileDirections* leftovers);

  bool ProcessReconcileAnonize(
      ledger::BalancePtr info,
      ledger::RewardsType type,
      double* fee,
      ledger::ReconcileDirections directions,
      ledger::ReconcileDirections* leftovers);

  void ProcessReconcile(
      ledger::ContributionQueuePtr contribution,
      ledger::BalancePtr info);

  void DeleteContributionQueue(ledger::ContributionQueuePtr contribution);

  void AdjustTipsAmounts(
    ledger::ReconcileDirections original_directions,
    ledger::ReconcileDirections* primary_directions,
    ledger::ReconcileDirections* rest_directions,
    double reduce_fee_for);

  void OnExternalWallets(
      const std::string& viewing_id,
      base::flat_map<std::string, double> wallet_balances,
      std::map<std::string, ledger::ExternalWalletPtr> wallets);

  void OnExternalWalletServerPublisherInfo(
      ledger::ServerPublisherInfoPtr info,
      const std::string& viewing_id,
      double amount,
      const ledger::ExternalWallet& wallet);

  void OnUpholdAC(ledger::Result result,
                  bool created,
                  const std::string& viewing_id);

  void OnDeleteContributionQueue(const ledger::Result result);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<PhaseOne> phase_one_;
  std::unique_ptr<PhaseTwo> phase_two_;
  std::unique_ptr<Unverified> unverified_;
  std::unique_ptr<Unblinded> unblinded_;
  std::unique_ptr<braveledger_uphold::Uphold> uphold_;
  uint32_t last_reconcile_timer_id_;
  std::map<std::string, uint32_t> retry_timers_;
  uint32_t queue_timer_id_;
  bool queue_in_progress_ = false;

  // For testing purposes
  friend class ContributionTest;
  FRIEND_TEST_ALL_PREFIXES(ContributionTest, GetAmountFromVerifiedAuto);
  FRIEND_TEST_ALL_PREFIXES(ContributionTest, GetTotalFromRecurringVerified);
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_H_
