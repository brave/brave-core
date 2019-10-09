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
#include "bat/ledger/internal/bat_helper.h"

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

namespace braveledger_contribution {
class PhaseOne;
}

namespace braveledger_contribution {
class PhaseTwo;
}

namespace braveledger_contribution {
class Unverified;
}

namespace braveledger_uphold {
class Uphold;
}

namespace braveledger_contribution {

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
  void InitReconcile(
      const ledger::RewardsType type,
      const braveledger_bat_helper::PublisherList& list,
      const braveledger_bat_helper::Directions& directions = {},
      double budget = 0);

  // Called when timer is triggered
  void OnTimer(uint32_t timer_id);

  // Sets new reconcile timer for monthly contribution in 30 days
  void SetReconcileTimer();

  // Does final stage in contribution
  // Sets reports and contribution info
  void OnReconcileCompleteSuccess(
      const std::string& viewing_id,
      const ledger::RewardsType type,
      const std::string& probi,
      ledger::ACTIVITY_MONTH month,
      int year,
      uint32_t date);
  void HasSufficientBalance(
    ledger::HasSufficientBalanceToReconcileCallback callback);

  // Fetches recurring tips that will be then used for the contribution.
  // This is called from global timer in impl.
  // Can be also called manually
  void StartMonthlyContribution();

  bool ShouldStartAutoContribute();

  void SetTimer(uint32_t* timer_id, uint64_t start_timer_in = 0);

  void AddRetry(
    ledger::ContributionRetry step,
    const std::string& viewing_id,
    braveledger_bat_helper::CURRENT_RECONCILE reconcile = {});

  // Resets reconcile stamps
  void ResetReconcileStamp();

  // Triggers contribution process for auto contribute table
  void StartAutoContribute();

  void ContributeUnverifiedPublishers();

  void StartPhaseTwo(const std::string& viewing_id);

  void DoDirectTip(
      const std::string& publisher_key,
      int amount,
      const std::string& currency,
      ledger::DoDirectTipCallback callback);

 private:
  // AUTO CONTRIBUTE: from the list gets only verified publishers and
  // save unverified to the db
  ledger::PublisherInfoList GetVerifiedListAuto(
      const ledger::PublisherInfoList& all,
      double* budget);

  // RECURRING TIPS: from the list gets only verified publishers and
  // save unverified to the db
  ledger::PublisherInfoList GetVerifiedListRecurring(
      const ledger::PublisherInfoList& all,
      double* budget);

  void PrepareACList(ledger::PublisherInfoList list,
                     uint32_t next_record);

  void PrepareRecurringList(ledger::PublisherInfoList list,
                            uint32_t next_record);

  void OnBalanceForReconcile(
      const ledger::RewardsType type,
      const braveledger_bat_helper::PublisherList& list,
      const braveledger_bat_helper::Directions& directions,
      double budget,
      const ledger::Result result,
      ledger::BalancePtr info);

  uint64_t GetRetryTimer(ledger::ContributionRetry step,
                         const std::string& viewing_id,
                         braveledger_bat_helper::CURRENT_RECONCILE* reconcile);

  int GetRetryPhase(ledger::ContributionRetry step);

  void DoRetry(const std::string& viewing_id);

  void GetVerifiedAutoAmount(
      const ledger::PublisherInfoList& publisher_list,
      uint32_t record,
      double balance,
      ledger::HasSufficientBalanceToReconcileCallback callback);

  void GetVerifiedRecurringAmount(
      const ledger::PublisherInfoList& publisher_list,
      uint32_t record,
      double balance,
      double budget,
      ledger::HasSufficientBalanceToReconcileCallback callback);

  double GetAmountFromVerifiedAuto(
      const ledger::PublisherInfoList& publisher_list,
      double ac_amount);

  static double GetAmountFromVerifiedRecurring(
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
    int amount,
    const std::string& currency,
    ledger::DoDirectTipCallback callback);

  bool HaveReconcileEnoughFunds(
      const ledger::RewardsType type,
      double* fee,
      double budget,
      double balance,
      const braveledger_bat_helper::Directions& directions);

  bool IsListEmpty(
    const ledger::RewardsType type,
    const braveledger_bat_helper::PublisherList& list,
    const braveledger_bat_helper::Directions& directions,
    double budget);

  void ProcessReconcile(
    const ledger::RewardsType type,
    const braveledger_bat_helper::PublisherList& list,
    const braveledger_bat_helper::Directions& directions,
    double budget,
    ledger::BalancePtr info);

  void AdjustTipsAmounts(
    braveledger_bat_helper::Directions directions,
    braveledger_bat_helper::Directions* wallet_directions,
    braveledger_bat_helper::Directions* anon_directions,
    double reduce_fee_for);

  void OnExternalWallets(
      const std::string& viewing_id,
      base::flat_map<std::string, double> wallet_balances,
      std::map<std::string, ledger::ExternalWalletPtr> wallets);

  void OnExternalWalletServerPublisherInfo(
      ledger::ServerPublisherInfoPtr info,
      const std::string& viewing_id,
      int amount,
      const ledger::ExternalWallet& wallet);

  void OnUpholdAC(ledger::Result result,
                  bool created,
                  const std::string& viewing_id);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<PhaseOne> phase_one_;
  std::unique_ptr<PhaseTwo> phase_two_;
  std::unique_ptr<Unverified> unverified_;
  std::unique_ptr<braveledger_uphold::Uphold> uphold_;
  uint32_t last_reconcile_timer_id_;
  std::map<std::string, uint32_t> retry_timers_;
  uint32_t delay_ac_timer_id;

  // For testing purposes
  friend class ContributionTest;
  FRIEND_TEST_ALL_PREFIXES(ContributionTest, GetAmountFromVerifiedAuto);
  FRIEND_TEST_ALL_PREFIXES(ContributionTest, GetAmountFromVerifiedRecurring);
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_H_
