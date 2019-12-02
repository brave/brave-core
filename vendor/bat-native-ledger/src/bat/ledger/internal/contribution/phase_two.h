/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_PHASE_TWO_H_
#define BRAVELEDGER_CONTRIBUTION_PHASE_TWO_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/contribution/contribution.h"
#include "bat/ledger/internal/properties/ballot_properties.h"
#include "bat/ledger/internal/properties/transaction_properties.h"
#include "bat/ledger/internal/properties/reconcile_direction_properties.h"
#include "bat/ledger/internal/properties/winner_properties.h"
#include "bat/ledger/internal/properties/batch_proof_properties.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_contribution {

class PhaseTwo {
 public:
  explicit PhaseTwo(
      bat_ledger::LedgerImpl* ledger,
      Contribution* contribution);

  ~PhaseTwo();

  void Initialize();

  void Start(const std::string& viewing_id);

  void PrepareBallots();

  void VoteBatch();

  void Proof();

  void OnTimer(uint32_t timer_id);

 private:
  unsigned int GetBallotsCount(const std::string& viewing_id);

  bool GetStatisticalVotingWinner(
      double dart,
      const ledger::ReconcileDirections& list,
      ledger::WinnerProperties* winner);

  ledger::Winners GetStatisticalVotingWinners(
      uint32_t total_votes,
      const ledger::ReconcileDirections& list);

  void GetContributeWinners(
      const unsigned int ballots,
      const std::string& viewing_id,
      const ledger::ReconcileDirections& list);

  void GetTipsWinners(const unsigned int ballots,
                      const std::string& viewing_id);

  void VotePublishers(const ledger::Winners& winners,
                      const std::string& viewing_id);

  void VotePublisher(const std::string& publisher,
                     const std::string& viewing_id);

  void PrepareBatch(
      const ledger::BallotProperties& ballot,
      const ledger::TransactionProperties& transaction);

  void PrepareBatchCallback(
      const std::string& viewing_id,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  static void AssignPrepareBallots(
      const std::string& viewing_id,
      const std::vector<std::string>& surveyors,
      ledger::Ballots* ballots);

  std::vector<std::string> ProofBatch(
      const ledger::BatchProofs& batch_proofs);

  void PrepareVoteBatch();

  static void AssignProofs(
      const ledger::BatchProofs& batch_proofs,
      const std::vector<std::string>& proofs,
      ledger::Ballots* ballots);

  void ProofBatchCallback(
      const ledger::BatchProofs& batch_proofs,
      const std::vector<std::string>& proofs);

  void VoteBatchCallback(
      const std::string& publisher,
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Contribution* contribution_;   // NOT OWNED
  uint32_t last_prepare_vote_batch_timer_id_;
  uint32_t last_vote_batch_timer_id_;

  // For testing purposes
  friend class PhaseTwoTest;
  FRIEND_TEST_ALL_PREFIXES(PhaseTwoTest, AssignPrepareBallotsRespectsViewingID);
  FRIEND_TEST_ALL_PREFIXES(PhaseTwoTest, AssignProofsRespectsViewingID);
  FRIEND_TEST_ALL_PREFIXES(PhaseTwoTest, GetStatisticalVotingWinners);
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_PHASE_TWO_H_
