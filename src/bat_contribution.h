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

namespace bat_ledger {
  class LedgerImpl;
}

namespace braveledger_bat_contribution {

class BatContribution {
 public:
  explicit BatContribution(bat_ledger::LedgerImpl* ledger);

  ~BatContribution();

  void Reconcile(
      const std::string &viewing_id,
      const ledger::PUBLISHER_CATEGORY category,
      const braveledger_bat_helper::PublisherList& list,
      const braveledger_bat_helper::Directions& directions = {});

  void OnTimer(uint32_t timer_id);

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

  void ViewingCredentials(const std::string& viewing_id,
                          const std::string& proof_stringified,
                          const std::string& anonize_viewing_id);

  void ViewingCredentialsCallback(
      const std::string& viewing_id,
      bool result,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnReconcileComplete(ledger::Result result,
                           const std::string& viewing_id,
                           const std::string& probi = "0");

  unsigned int GetBallotsCount(const std::string& viewing_id);

  void GetReconcileWinners(const unsigned int& ballots,
                           const std::string& viewing_id);

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

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  bat_ledger::URLRequestHandler handler_;
  uint32_t last_reconcile_timer_id_;
  uint32_t last_prepare_vote_batch_timer_id_;
  uint32_t last_vote_batch_timer_id_;
};

}  // namespace braveledger_bat_contribution
#endif  // BRAVELEDGER_BAT_CONTRIBUTION_H_
