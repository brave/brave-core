/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BRAVELEDGER_BAT_CONTRIBUTION_H_
#define BRAVELEDGER_BAT_CONTRIBUTION_H_

#include <string>
#include <map>

#include "bat/ledger/ledger.h"
#include "bat_helper.h"
#include "url_request_handler.h"

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

// PHASE 1 (reconcile)
// 1. StartReconcile
// 2. Reconcile
// 3. ReconcileCallback
// 4. CurrentReconcile
// 5. CurrentReconcileCallback
// 6. ReconcilePayload
// 7. ReconcilePayloadCallback
// 8. RegisterViewing
// 9. RegisterViewingCallback
// 10. ViewingCredentials
// 11. ViewingCredentialsCallback
// 12. OnReconcileComplete

// PHASE 2 (voting)
// 1. GetReconcileWinners
// 2. VotePublishers
// 3. VotePublisher
// 4. PrepareBallots
// 5. PrepareBatch
// 6. PrepareBatchCallback
// 7. ProofBatch
// 8. ProofBatchCallback
// 9. SetTimer
// 10. PrepareVoteBatch
// 11. SetTimer
// 12. VoteBatch
// 13. VoteBatchCallback
// 14. SetTimer - we set timer until the whole batch is processed

namespace bat_ledger {
  class LedgerImpl;
}

namespace braveledger_bat_contribution {

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

//static const uint64_t phase_one_timers[] = {
//    0.5 * 60,  // 30
//    1 * 60,  //  1min
//    2 * 60,  //  2min
//    3 * 60,  // 3min
//    4 * 60};  // 4min
//
//static const uint64_t phase_two_timers[] = {
//    1 * 60,  // 1min
//    2 * 60,  //  2min
//    3 * 60};  // 3min

class BatContribution {
 public:
  explicit BatContribution(bat_ledger::LedgerImpl* ledger);

  ~BatContribution();

  void OnStartUp();

  // Starting point for contribution
  // We determinate which contribution we want to do and do appropriate actions
  void StartReconcile(
      const std::string &viewing_id,
      const ledger::PUBLISHER_CATEGORY category,
      const braveledger_bat_helper::PublisherList& list,
      const braveledger_bat_helper::Directions& directions = {});

  // Called when timer is triggered
  void OnTimer(uint32_t timer_id);

  // Sets new reconcile timer for monthly contribution in 30 days
  void SetReconcileTimer();

  // Does final stage in contribution
  // Sets reports and contribution info
  void OnReconcileCompleteSuccess(const std::string& viewing_id,
                                  ledger::PUBLISHER_CATEGORY category,
                                  const std::string& probi,
                                  ledger::PUBLISHER_MONTH month,
                                  int year,
                                  uint32_t date);

 private:
  std::string GetAnonizeProof(const std::string& registrar_VK,
                              const std::string& id,
                              std::string& pre_flight);

  // Entry point for contribution where we have publisher info list
  void ReconcilePublisherList(ledger::PUBLISHER_CATEGORY category,
                              const ledger::PublisherInfoList& list,
                              uint32_t next_record);

  // Fetches recurring donations that will be then used for the contribution.
  // This is called from global timer in impl.
  void OnTimerReconcile();

  // Triggers contribution process for auto contribute table
  void StartAutoContribute();

  void Reconcile(const std::string &viewing_id);

  void ReconcileCallback(const std::string& viewing_id,
                         bool result,
                         const std::string& response,
                         const std::map<std::string, std::string>& headers);

  void CurrentReconcile(const std::string& viewing_id);

  void CurrentReconcileCallback(
      const std::string& viewing_id,
      bool result,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void ReconcilePayload(const std::string& viewing_id);

  void ReconcilePayloadCallback(
      const std::string& viewing_id,
      bool result,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void RegisterViewing(const std::string& viewing_id);

  void RegisterViewingCallback(
      const std::string& viewing_id,
      bool result,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void ViewingCredentials(const std::string& viewing_id);

  void ViewingCredentialsCallback(
      const std::string& viewing_id,
      bool result,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnReconcileComplete(ledger::Result result,
                           const std::string& viewing_id,
                           int category,
                           const std::string& probi = "0");

  unsigned int GetBallotsCount(const std::string& viewing_id);

  void GetReconcileWinners(const std::string& viewing_id);

  void GetContributeWinners(const unsigned int& ballots,
                            const std::string& viewing_id,
                            const braveledger_bat_helper::PublisherList& list);

  void GetDonationWinners(const unsigned int& ballots,
                          const std::string& viewing_id,
                          const braveledger_bat_helper::PublisherList& list);

  void VotePublishers(const braveledger_bat_helper::Winners& winners,
                      const std::string& viewing_id);

  void VotePublisher(const std::string& publisher,
                     const std::string& viewing_id);

  void PrepareBallots();

  void PrepareBatch(
      const braveledger_bat_helper::BALLOT_ST& ballot,
      const braveledger_bat_helper::TRANSACTION_ST& transaction);

  void PrepareBatchCallback(
      bool result,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void Proof();

  void ProofBatch(
      const braveledger_bat_helper::BathProofs& batch_proof,
      ledger::LedgerTaskRunner::CallerThreadCallback callback);

  void ProofBatchCallback(
      const braveledger_bat_helper::BathProofs& batch_proof,
      const std::vector<std::string>& proofs);

  void PrepareVoteBatch();

  void VoteBatch();

  void VoteBatchCallback(
      const std::string& publisher,
      bool result,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void SetTimer(uint32_t& timer_id, uint64_t start_timer_in = 0);

  void AddRetry(
    braveledger_bat_helper::ContributionRetry step,
    const std::string& viewing_id,
    braveledger_bat_helper::CURRENT_RECONCILE reconcile = {});

  uint64_t GetRetryTimer(braveledger_bat_helper::ContributionRetry step,
                         const std::string& viewing_id,
                         braveledger_bat_helper::CURRENT_RECONCILE& reconcile);

  int GetRetryPhase(braveledger_bat_helper::ContributionRetry step);

  void DoRetry(const std::string& viewing_id);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  bat_ledger::URLRequestHandler handler_;
  uint32_t last_reconcile_timer_id_;
  uint32_t last_prepare_vote_batch_timer_id_;
  uint32_t last_vote_batch_timer_id_;
  std::map<std::string, uint32_t> retry_timers_;
};

}  // namespace braveledger_bat_contribution
#endif  // BRAVELEDGER_BAT_CONTRIBUTION_H_
