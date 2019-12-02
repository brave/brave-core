/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contribution/phase_two.h"

#include "anon/anon.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/properties/ballot_properties.h"
#include "bat/ledger/internal/properties/batch_proof_properties.h"
#include "bat/ledger/internal/properties/reconcile_direction_properties.h"
#include "bat/ledger/internal/properties/transaction_ballot_properties.h"
#include "bat/ledger/internal/properties/transaction_properties.h"
#include "bat/ledger/internal/properties/winner_properties.h"
#include "bat/ledger/internal/state/publisher_vote_state.h"
#include "bat/ledger/internal/state/surveyor_state.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"

#if defined(OS_IOS)
#include <dispatch/dispatch.h>
#endif

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_contribution {

PhaseTwo::PhaseTwo(bat_ledger::LedgerImpl* ledger,
    Contribution* contribution) :
    ledger_(ledger),
    contribution_(contribution),
    last_prepare_vote_batch_timer_id_(0u),
    last_vote_batch_timer_id_(0u) {
}

PhaseTwo::~PhaseTwo() {
}

void PhaseTwo::Initialize() {
  // Check if we have some more pending ballots to go out
  PrepareBallots();
}

void PhaseTwo::Start(const std::string& viewing_id) {
  unsigned int ballots_count = GetBallotsCount(viewing_id);
  const auto reconcile = ledger_->GetReconcileById(viewing_id);

  switch (reconcile.type) {
    case ledger::RewardsType::AUTO_CONTRIBUTE: {
      GetContributeWinners(ballots_count, viewing_id, reconcile.directions);
      break;
    }

    case ledger::RewardsType::RECURRING_TIP:
    case ledger::RewardsType::ONE_TIME_TIP: {
      ledger::WinnerProperties winner;
      winner.vote_count = ballots_count;
      winner.direction.publisher_key =
          reconcile.directions.front().publisher_key;
      winner.direction.amount_percent = 100.0;
      VotePublishers(ledger::Winners { winner }, viewing_id);
      break;
    }

    default:
      // TODO(nejczdovc) what should we do here?
      return;
  }
}

unsigned int PhaseTwo::GetBallotsCount(
    const std::string& viewing_id) {
  unsigned int count = 0;
  ledger::Transactions transactions = ledger_->GetTransactions();
  for (size_t i = 0; i < transactions.size(); i++) {
    if (transactions[i].vote_count < transactions[i].surveyor_ids.size()
        && transactions[i].viewing_id == viewing_id) {
      count += transactions[i].surveyor_ids.size() - transactions[i].vote_count;
    }
  }

  return count;
}

bool PhaseTwo::GetStatisticalVotingWinner(
    double dart,
    const ledger::ReconcileDirections& directions,
    ledger::WinnerProperties* winner) {
  double upper = 0.0;
  for (const auto& direction : directions) {
    upper += direction.amount_percent / 100.0;
    if (upper < dart)
      continue;

    winner->vote_count = 1;
    winner->direction = direction;

    return true;
  }

  return false;
}

ledger::Winners PhaseTwo::GetStatisticalVotingWinners(
    uint32_t total_votes,
    const ledger::ReconcileDirections& directions) {
  ledger::Winners winners;

  while (total_votes > 0) {
    double dart = brave_base::random::Uniform_01();
    ledger::WinnerProperties winner;
    if (GetStatisticalVotingWinner(dart, directions, &winner)) {
      winners.push_back(winner);
      --total_votes;
    }
  }

  return winners;
}

void PhaseTwo::GetContributeWinners(
    const unsigned int ballots,
    const std::string& viewing_id,
    const ledger::ReconcileDirections& directions) {
  ledger::Winners winners = GetStatisticalVotingWinners(ballots, directions);
  VotePublishers(winners, viewing_id);
}

void PhaseTwo::VotePublishers(
    const ledger::Winners& winners,
    const std::string& viewing_id) {
  std::vector<std::string> publishers;
  for (size_t i = 0; i < winners.size(); i++) {
    for (size_t j = 0; j < winners[i].vote_count; j++) {
      publishers.push_back(winners[i].direction.publisher_key);
    }
  }

  for (size_t i = 0; i < publishers.size(); i++) {
    VotePublisher(publishers[i], viewing_id);
  }

  ledger_->AddReconcileStep(viewing_id, ledger::ContributionRetry::STEP_FINAL);

  PrepareBallots();
}

void PhaseTwo::VotePublisher(const std::string& publisher,
                                    const std::string& viewing_id) {
  DCHECK(!publisher.empty());
  if (publisher.empty()) {
    // TODO(nejczdovc) what should we do in this case?
    return;
  }

  ledger::BallotProperties ballot;
  int i = 0;

  ledger::Transactions transactions = ledger_->GetTransactions();

  if (transactions.size() == 0) {
    // TODO(nejczdovc) what should we do in this case?
    return;
  }

  for (i = transactions.size() - 1; i >=0; i--) {
    if (transactions[i].vote_count >= transactions[i].surveyor_ids.size()) {
      continue;
    }

    if (transactions[i].viewing_id == viewing_id || viewing_id.empty()) {
      break;
    }
  }

  // transaction was not found
  if (i < 0) {
    // TODO(nejczdovc) what should we do in this case?
    return;
  }

  ballot.viewing_id = transactions[i].viewing_id;
  ballot.surveyor_id = transactions[i].surveyor_ids[transactions[i].vote_count];
  ballot.publisher = publisher;
  ballot.count = transactions[i].vote_count;
  transactions[i].vote_count++;

  ledger::Ballots ballots = ledger_->GetBallots();
  ballots.push_back(ballot);

  ledger_->SetTransactions(transactions);
  ledger_->SetBallots(ballots);
}

void PhaseTwo::PrepareBallots() {
  ledger::Transactions transactions = ledger_->GetTransactions();
  ledger::Ballots ballots = ledger_->GetBallots();

  if (ballots.size() == 0) {
    // skip ballots and start sending votes
    contribution_->SetTimer(&last_vote_batch_timer_id_);
    return;
  }

  for (int i = ballots.size() - 1; i >= 0; i--) {
    for (size_t j = 0; j < transactions.size(); j++) {
      if (transactions[j].viewing_id == ballots[i].viewing_id) {
        if (ballots[i].prepare_ballot.empty()) {
          PrepareBatch(ballots[i], transactions[j]);
          return;
        }

        if (ballots[i].proof_ballot.empty()) {
          Proof();
          return;
        }
      }
    }
  }

  // In case we already prepared all ballots
  PrepareVoteBatch();
}

void PhaseTwo::PrepareBatch(
    const ledger::BallotProperties& ballot,
    const ledger::TransactionProperties& transaction) {
  std::string url = braveledger_request_util::BuildUrl(
      (std::string)SURVEYOR_BATCH_VOTING +
      "/" +
      transaction.anonize_viewing_id, PREFIX_V2);

  auto callback = std::bind(&PhaseTwo::PrepareBatchCallback,
                            this,
                            transaction.viewing_id,
                            _1,
                            _2,
                            _3);
  ledger_->LoadURL(url,
      std::vector<std::string>(),
      "",
      "",
      ledger::UrlMethod::GET,
      callback);
}

void PhaseTwo::AssignPrepareBallots(
    const std::string& viewing_id,
    const std::vector<std::string>& surveyors,
    ledger::Ballots* ballots) {
  for (size_t j = 0; j < surveyors.size(); j++) {
    std::string error;
    braveledger_bat_helper::getJSONValue("error", surveyors[j], &error);
    if (!error.empty()) {
      // TODO(nejczdovc) what should we do here
      continue;
    }

    std::string surveyor_id;
    bool success = braveledger_bat_helper::getJSONValue("surveyorId",
                                                        surveyors[j],
                                                        &surveyor_id);
    if (!success) {
      // TODO(nejczdovc) what should we do here
      continue;
    }

    for (auto& ballot : *ballots) {
      if (ballot.surveyor_id == surveyor_id &&
          ballot.viewing_id == viewing_id) {
        ballot.prepare_ballot = surveyors[j];
      }
    }
  }
}

void PhaseTwo::PrepareBatchCallback(
    const std::string& viewing_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_PREPARE, "");
    return;
  }

  std::vector<std::string> surveyors;
  bool success = braveledger_bat_helper::getJSONBatchSurveyors(response,
                                                               &surveyors);
  if (!success) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_PREPARE, "");
    return;
  }

  ledger::Ballots ballots = ledger_->GetBallots();

  AssignPrepareBallots(viewing_id, surveyors, &ballots);

  ledger_->SetBallots(ballots);
  Proof();
}

void PhaseTwo::Proof() {
  ledger::BatchProofs batch_proofs;

  ledger::Transactions transactions = ledger_->GetTransactions();
  ledger::Ballots ballots = ledger_->GetBallots();

  for (int i = ballots.size() - 1; i >= 0; i--) {
    for (size_t k = 0; k < transactions.size(); k++) {
      if (transactions[k].viewing_id == ballots[i].viewing_id) {
        if (ballots[i].prepare_ballot.empty()) {
          // TODO(nejczdovc) what should we do here
          return;
        }

        if (ballots[i].proof_ballot.empty()) {
          ledger::BatchProofProperties batch_proof;
          batch_proof.transaction = transactions[k];
          batch_proof.ballot = ballots[i];
          batch_proofs.push_back(batch_proof);
        }
      }
    }
  }

#if defined(OS_IOS)
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT,
                                           0), ^{
    const auto result = this->ProofBatch(batch_proofs);
    dispatch_async(dispatch_get_main_queue(), ^{
      this->ProofBatchCallback(batch_proofs, result);
    });
  });
#else
  base::PostTaskAndReplyWithResult(
      ledger_->GetTaskRunner().get(),
      FROM_HERE,
      base::BindOnce(&PhaseTwo::ProofBatch,
        base::Unretained(this),
        batch_proofs),
      base::BindOnce(&PhaseTwo::ProofBatchCallback,
        base::Unretained(this),
        batch_proofs));
#endif
}

std::vector<std::string> PhaseTwo::ProofBatch(
    const ledger::BatchProofs& batch_proofs) {
  std::vector<std::string> proofs;

  for (size_t i = 0; i < batch_proofs.size(); i++) {
    ledger::SurveyorProperties surveyor;
    const ledger::SurveyorState surveyor_state;
    bool success = surveyor_state.FromJson(
        batch_proofs[i].ballot.prepare_ballot, &surveyor);

    if (!success) {
      BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
        "Failed to load surveyor state: " <<
            batch_proofs[i].ballot.prepare_ballot;
      continue;
    }

    std::string signature_to_send;
    size_t delimeter_pos = surveyor.signature.find(',');
    if (std::string::npos != delimeter_pos &&
        delimeter_pos + 1 <= surveyor.signature.length()) {
      signature_to_send = surveyor.signature.substr(delimeter_pos + 1);

      if (signature_to_send.length() > 1 && signature_to_send[0] == ' ') {
        signature_to_send.erase(0, 1);
      }
    }

    if (signature_to_send.empty()) {
      continue;
    }

    std::string msg_key[1] = {"publisher"};
    std::string msg_value[1] = {batch_proofs[i].ballot.publisher};
    std::string msg = braveledger_bat_helper::stringify(msg_key, msg_value, 1);

    const char* proof = submitMessage(
        msg.c_str(),
        batch_proofs[i].transaction.master_user_token.c_str(),
        batch_proofs[i].transaction.registrar_vk.c_str(),
        signature_to_send.c_str(),
        surveyor.surveyor_id.c_str(),
        surveyor.survey_vk.c_str());

    std::string annon_proof;
    if (proof != nullptr) {
      annon_proof = proof;
      // should fix in
      // https://github.com/brave-intl/bat-native-anonize/issues/11
      free((void*)proof); // NOLINT
    }

    proofs.push_back(annon_proof);
  }

  return proofs;
}

void PhaseTwo::AssignProofs(
    const ledger::BatchProofs& batch_proofs,
    const std::vector<std::string>& proofs,
    ledger::Ballots* ballots) {
  for (size_t i = 0; i < batch_proofs.size(); i++) {
    for (auto& ballot : *ballots) {
      if (ballot.surveyor_id == batch_proofs[i].ballot.surveyor_id &&
          ballot.viewing_id == batch_proofs[i].ballot.viewing_id) {
        ballot.proof_ballot = proofs[i];
      }
    }
  }
}

void PhaseTwo::ProofBatchCallback(
    const ledger::BatchProofs& batch_proofs,
    const std::vector<std::string>& proofs) {
  ledger::Ballots ballots = ledger_->GetBallots();

  AssignProofs(batch_proofs, proofs, &ballots);

  ledger_->SetBallots(ballots);

  if (batch_proofs.size() != proofs.size()) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_PROOF, "");
    return;
  }

  contribution_->SetTimer(&last_prepare_vote_batch_timer_id_);
}

void PhaseTwo::PrepareVoteBatch() {
  ledger::Transactions transactions = ledger_->GetTransactions();
  ledger::Ballots ballots = ledger_->GetBallots();
  ledger::PublisherVotes publisher_votes = ledger_->GetPublisherVotes();

  if (ballots.size() == 0) {
    contribution_->SetTimer(&last_vote_batch_timer_id_);
    return;
  }

  for (int i = ballots.size() - 1; i >= 0; i--) {
    if (ballots[i].prepare_ballot.empty() || ballots[i].proof_ballot.empty()) {
      // TODO(nejczdovc) what to do in this case
      continue;
    }

    bool transaction_exit = false;
    for (size_t k = 0; k < transactions.size(); k++) {
      if (transactions[k].viewing_id == ballots[i].viewing_id) {
        bool ballot_exit = false;
        for (size_t j = 0; j < transactions[k].transaction_ballots.size();
            j++) {
          if (transactions[k].transaction_ballots[j].publisher ==
              ballots[i].publisher) {
            transactions[k].transaction_ballots[j].count++;
            ballot_exit = true;
            break;
          }
        }

        if (!ballot_exit) {
          ledger::TransactionBallotProperties transactionBallot;
          transactionBallot.publisher = ballots[i].publisher;
          transactionBallot.count++;
          transactions[k].transaction_ballots.push_back(transactionBallot);
        }
        transaction_exit = true;
        break;
      }
    }

    if (!transaction_exit) {
      // TODO(nejczdovc) what to do in this case
      continue;
    }

    bool exist_batch = false;
    ledger::PublisherVoteProperties publisher_vote;
    publisher_vote.surveyor_id = ballots[i].surveyor_id;
    publisher_vote.proof = ballots[i].proof_ballot;

    for (size_t k = 0; k < publisher_votes.size(); k++) {
      if (publisher_votes[k].publisher == ballots[i].publisher) {
        exist_batch = true;
        publisher_votes[k].batch_votes.push_back(publisher_vote);
      }
    }

    if (!exist_batch) {
      ledger::PublisherVotesProperties new_publisher_votes;
      new_publisher_votes.publisher = ballots[i].publisher;
      new_publisher_votes.batch_votes.push_back(publisher_vote);
      publisher_votes.push_back(new_publisher_votes);
    }

    ballots.erase(ballots.begin() + i);
  }

  ledger_->SetTransactions(transactions);
  ledger_->SetBallots(ballots);
  ledger_->SetPublisherVotes(publisher_votes);
  contribution_->SetTimer(&last_vote_batch_timer_id_);
}

void PhaseTwo::VoteBatch() {
  ledger::PublisherVotes publisher_votes = ledger_->GetPublisherVotes();
  if (publisher_votes.size() == 0) {
    return;
  }

  ledger::PublisherVotesProperties publisher_votes_properties =
      publisher_votes[0];
  ledger::BatchVotes batch_votes;

  if (publisher_votes_properties.batch_votes.size() > VOTE_BATCH_SIZE) {
    batch_votes.assign(publisher_votes_properties.batch_votes.begin(),
        publisher_votes_properties.batch_votes.begin() + VOTE_BATCH_SIZE);
  } else {
    batch_votes = publisher_votes_properties.batch_votes;
  }

  const ledger::PublisherVoteState publisher_vote_state;
  std::string payload = publisher_vote_state.ToJson(batch_votes);

  std::string url = braveledger_request_util::BuildUrl(
      (std::string)SURVEYOR_BATCH_VOTING,
          PREFIX_V2);
  auto callback = std::bind(&PhaseTwo::VoteBatchCallback,
                            this,
                            publisher_votes_properties.publisher,
                            _1,
                            _2,
                            _3);
  ledger_->LoadURL(url,
      std::vector<std::string>(),
      payload,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      callback);
}

void PhaseTwo::VoteBatchCallback(
    const std::string& publisher,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_VOTE, "");
    return;
  }

  std::vector<std::string> surveyors;
  bool success = braveledger_bat_helper::getJSONBatchSurveyors(response,
                                                               &surveyors);
  if (!success) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_VOTE, "");
    return;
  }

  ledger::PublisherVotes publisher_votes = ledger_->GetPublisherVotes();

  for (size_t i = 0; i < publisher_votes.size(); i++) {
    if (publisher_votes[i].publisher == publisher) {
      size_t sizeToCheck = VOTE_BATCH_SIZE;
      if (publisher_votes[i].batch_votes.size() < VOTE_BATCH_SIZE) {
        sizeToCheck = publisher_votes[i].batch_votes.size();
      }

      for (int j = sizeToCheck - 1; j >= 0; j--) {
        for (size_t k = 0; k < surveyors.size(); k++) {
          std::string surveyor_id;
          bool success = braveledger_bat_helper::getJSONValue("surveyorId",
                                                              surveyors[k],
                                                              &surveyor_id);
          if (!success) {
            // TODO(nejczdovc) what to do in this case
            continue;
          }

          if (surveyor_id == publisher_votes[i].batch_votes[j].surveyor_id) {
            publisher_votes[i].batch_votes.erase(
                publisher_votes[i].batch_votes.begin() + j);
            break;
          }
        }
      }

      if (publisher_votes[i].batch_votes.size() == 0) {
        publisher_votes.erase(publisher_votes.begin() + i);
      }
      break;
    }
  }

  ledger_->SetPublisherVotes(publisher_votes);

  if (publisher_votes.size() > 0) {
    contribution_->SetTimer(&last_vote_batch_timer_id_);
  }
}

void PhaseTwo::OnTimer(uint32_t timer_id) {
  if (timer_id == last_prepare_vote_batch_timer_id_) {
    last_prepare_vote_batch_timer_id_ = 0;
    PrepareVoteBatch();
    return;
  }

  if (timer_id == last_vote_batch_timer_id_) {
    last_vote_batch_timer_id_ = 0;
    VoteBatch();
    return;
  }
}

}  // namespace braveledger_contribution
