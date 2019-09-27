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
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/contribution/contribution.h"

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
      const braveledger_bat_helper::PublisherList& list,
      braveledger_bat_helper::WINNERS_ST* winner);

  braveledger_bat_helper::Winners GetStatisticalVotingWinners(
      uint32_t total_votes,
      const braveledger_bat_helper::PublisherList& list);

  void GetContributeWinners(const unsigned int ballots,
                            const std::string& viewing_id,
                            const braveledger_bat_helper::PublisherList& list);

  void GetTipsWinners(const unsigned int ballots,
                      const std::string& viewing_id);

  void VotePublishers(const braveledger_bat_helper::Winners& winners,
                      const std::string& viewing_id);

  void VotePublisher(const std::string& publisher,
                     const std::string& viewing_id);

  void PrepareBatch(
      const braveledger_bat_helper::BALLOT_ST& ballot,
      const braveledger_bat_helper::TRANSACTION_ST& transaction);

  void PrepareBatchCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  std::vector<std::string> ProofBatch(
      const braveledger_bat_helper::BatchProofs& batch_proofs);

  void PrepareVoteBatch();

  void ProofBatchCallback(
      const braveledger_bat_helper::BatchProofs& batch_proofs,
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
  FRIEND_TEST_ALL_PREFIXES(PhaseTwoTest, GetStatisticalVotingWinners);
};

}  // namespace braveledger_contribution
#endif  // BRAVELEDGER_CONTRIBUTION_PHASE_TWO_H_
