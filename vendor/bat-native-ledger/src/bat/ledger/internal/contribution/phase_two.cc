/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contribution/phase_two.h"

#include "anon/anon.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"

#if defined(OS_IOS)
#include <dispatch/dispatch.h>
#endif

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_contribution {

static bool winners_votes_compare(
    const braveledger_bat_helper::WINNERS_ST& first,
    const braveledger_bat_helper::WINNERS_ST& second) {
  return (first.votes_ < second.votes_);
}

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

  switch (reconcile.type_) {
    case ledger::RewardsType::AUTO_CONTRIBUTE: {
      GetContributeWinners(ballots_count, viewing_id, reconcile.list_);
      break;
    }

    case ledger::RewardsType::RECURRING_TIP: {
      GetTipsWinners(ballots_count, viewing_id);
      break;
    }

    case ledger::RewardsType::ONE_TIME_TIP: {
      // Direct one-time contribution
      braveledger_bat_helper::WINNERS_ST winner;
      winner.votes_ = ballots_count;
      winner.publisher_data_.id_ = reconcile.directions_.front().publisher_key_;
      winner.publisher_data_.duration_ = 0;
      winner.publisher_data_.score_ = 0;
      winner.publisher_data_.visits_ = 0;
      winner.publisher_data_.percent_ = 0;
      winner.publisher_data_.weight_ = 0;
      VotePublishers(braveledger_bat_helper::Winners { winner }, viewing_id);
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
  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();
  for (size_t i = 0; i < transactions.size(); i++) {
    if (transactions[i].votes_ < transactions[i].surveyorIds_.size()
        && transactions[i].viewingId_ == viewing_id) {
      count += transactions[i].surveyorIds_.size() - transactions[i].votes_;
    }
  }

  return count;
}

bool PhaseTwo::GetStatisticalVotingWinner(
    double dart,
    const braveledger_bat_helper::PublisherList& list,
    braveledger_bat_helper::WINNERS_ST* winner) {
  double upper = 0.0;
  for (const auto& item : list) {
    upper += item.weight_ / 100.0;
    if (upper < dart)
      continue;

    winner->votes_ = 1;
    winner->publisher_data_ = item;

    return true;
  }

  return false;
}

braveledger_bat_helper::Winners PhaseTwo::GetStatisticalVotingWinners(
    uint32_t total_votes,
    const braveledger_bat_helper::PublisherList& list) {
  braveledger_bat_helper::Winners winners;

  while (total_votes > 0) {
    double dart = brave_base::random::Uniform_01();
    braveledger_bat_helper::WINNERS_ST winner;
    if (GetStatisticalVotingWinner(dart, list, &winner)) {
      winners.push_back(winner);
      --total_votes;
    }
  }

  return winners;
}

void PhaseTwo::GetContributeWinners(
    const unsigned int ballots,
    const std::string& viewing_id,
    const braveledger_bat_helper::PublisherList& list) {
  braveledger_bat_helper::Winners winners =
      GetStatisticalVotingWinners(ballots, list);
  VotePublishers(winners, viewing_id);
}

void PhaseTwo::GetTipsWinners(
    const unsigned int ballots,
    const std::string& viewing_id) {
  const auto reconcile = ledger_->GetReconcileById(viewing_id);
  unsigned int total_votes = 0;
  braveledger_bat_helper::Winners res;

  for (const auto &item : reconcile.directions_) {
    if (item.amount_ <= 0) {
      continue;
    }

    braveledger_bat_helper::WINNERS_ST winner;
    double percent = item.amount_ / reconcile.fee_;
    winner.votes_ = static_cast<unsigned int>(std::lround(percent *
        static_cast<double>(ballots)));
    total_votes += winner.votes_;
    winner.publisher_data_.id_ = item.publisher_key_;
    winner.publisher_data_.duration_ = 0;
    winner.publisher_data_.score_ = 0;
    winner.publisher_data_.visits_ = 0;
    winner.publisher_data_.percent_ = 0;
    winner.publisher_data_.weight_ = 0;
    res.push_back(winner);
  }

  if (res.size()) {
    while (total_votes > ballots) {
      braveledger_bat_helper::Winners::iterator max =
          std::max_element(res.begin(), res.end(), winners_votes_compare);
      (max->votes_)--;
      total_votes--;
    }
  } else {
    // TODO(nejczdovc) what should we do in this case?
  }

  VotePublishers(res, viewing_id);
}

void PhaseTwo::VotePublishers(
    const braveledger_bat_helper::Winners& winners,
    const std::string& viewing_id) {
  std::vector<std::string> publishers;
  for (size_t i = 0; i < winners.size(); i++) {
    for (size_t j = 0; j < winners[i].votes_; j++) {
      publishers.push_back(winners[i].publisher_data_.id_);
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

  braveledger_bat_helper::BALLOT_ST ballot;
  int i = 0;

  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();

  if (transactions.size() == 0) {
    // TODO(nejczdovc) what should we do in this case?
    return;
  }

  for (i = transactions.size() - 1; i >=0; i--) {
    if (transactions[i].votes_ >= transactions[i].surveyorIds_.size()) {
      continue;
    }

    if (transactions[i].viewingId_ == viewing_id || viewing_id.empty()) {
      break;
    }
  }

  // transaction was not found
  if (i < 0) {
    // TODO(nejczdovc) what should we do in this case?
    return;
  }

  ballot.viewingId_ = transactions[i].viewingId_;
  ballot.surveyorId_ = transactions[i].surveyorIds_[transactions[i].votes_];
  ballot.publisher_ = publisher;
  ballot.offset_ = transactions[i].votes_;
  transactions[i].votes_++;

  braveledger_bat_helper::Ballots ballots = ledger_->GetBallots();
  ballots.push_back(ballot);

  ledger_->SetTransactions(transactions);
  ledger_->SetBallots(ballots);
}

void PhaseTwo::PrepareBallots() {
  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();
  braveledger_bat_helper::Ballots ballots = ledger_->GetBallots();

  if (ballots.size() == 0) {
    // skip ballots and start sending votes
    contribution_->SetTimer(&last_vote_batch_timer_id_);
    return;
  }

  for (int i = ballots.size() - 1; i >= 0; i--) {
    for (size_t j = 0; j < transactions.size(); j++) {
      if (transactions[j].viewingId_ == ballots[i].viewingId_) {
        if (ballots[i].prepareBallot_.empty()) {
          PrepareBatch(ballots[i], transactions[j]);
          return;
        }

        if (ballots[i].proofBallot_.empty()) {
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
    const braveledger_bat_helper::BALLOT_ST& ballot,
    const braveledger_bat_helper::TRANSACTION_ST& transaction) {
  std::string url = braveledger_bat_helper::buildURL(
      (std::string)SURVEYOR_BATCH_VOTING +
      "/" +
      transaction.anonizeViewingId_, PREFIX_V2);

  auto callback = std::bind(&PhaseTwo::PrepareBatchCallback,
                            this,
                            _1,
                            _2,
                            _3);
  ledger_->LoadURL(url,
      std::vector<std::string>(),
      "",
      "",
      ledger::URL_METHOD::GET,
      callback);
}

void PhaseTwo::PrepareBatchCallback(
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

  braveledger_bat_helper::Transactions transactions =
    ledger_->GetTransactions();
  braveledger_bat_helper::Ballots ballots = ledger_->GetBallots();

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

    for (int i = ballots.size() - 1; i >= 0; i--) {
      if (ballots[i].surveyorId_ == surveyor_id) {
        for (size_t k = 0; k < transactions.size(); k++) {
          if (transactions[k].viewingId_ == ballots[i].viewingId_ &&
              ballots[i].proofBallot_.empty()) {
            ballots[i].prepareBallot_ = surveyors[j];
          }
        }
      }
    }
  }

  ledger_->SetBallots(ballots);
  Proof();
}

void PhaseTwo::Proof() {
  braveledger_bat_helper::BatchProofs batch_proofs;

  braveledger_bat_helper::Transactions transactions =
    ledger_->GetTransactions();
  braveledger_bat_helper::Ballots ballots = ledger_->GetBallots();

  for (int i = ballots.size() - 1; i >= 0; i--) {
    for (size_t k = 0; k < transactions.size(); k++) {
      if (transactions[k].viewingId_ == ballots[i].viewingId_) {
        if (ballots[i].prepareBallot_.empty()) {
          // TODO(nejczdovc) what should we do here
          return;
        }

        if (ballots[i].proofBallot_.empty()) {
          braveledger_bat_helper::BATCH_PROOF batch_proof_el;
          batch_proof_el.transaction_ = transactions[k];
          batch_proof_el.ballot_ = ballots[i];
          batch_proofs.push_back(batch_proof_el);
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
    const braveledger_bat_helper::BatchProofs& batch_proofs) {
  std::vector<std::string> proofs;

  for (size_t i = 0; i < batch_proofs.size(); i++) {
    braveledger_bat_helper::SURVEYOR_ST surveyor;
    bool success = braveledger_bat_helper::loadFromJson(
        &surveyor,
        batch_proofs[i].ballot_.prepareBallot_);

    if (!success) {
      BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
        "Failed to load surveyor state: " <<
        batch_proofs[i].ballot_.prepareBallot_;
      continue;
    }

    std::string signature_to_send;
    size_t delimeter_pos = surveyor.signature_.find(',');
    if (std::string::npos != delimeter_pos &&
        delimeter_pos + 1 <= surveyor.signature_.length()) {
      signature_to_send = surveyor.signature_.substr(delimeter_pos + 1);

      if (signature_to_send.length() > 1 && signature_to_send[0] == ' ') {
        signature_to_send.erase(0, 1);
      }
    }

    if (signature_to_send.empty()) {
      continue;
    }

    std::string msg_key[1] = {"publisher"};
    std::string msg_value[1] = {batch_proofs[i].ballot_.publisher_};
    std::string msg = braveledger_bat_helper::stringify(msg_key, msg_value, 1);

    const char* proof = submitMessage(
        msg.c_str(),
        batch_proofs[i].transaction_.masterUserToken_.c_str(),
        batch_proofs[i].transaction_.registrarVK_.c_str(),
        signature_to_send.c_str(),
        surveyor.surveyorId_.c_str(),
        surveyor.surveyVK_.c_str());

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

void PhaseTwo::ProofBatchCallback(
    const braveledger_bat_helper::BatchProofs& batch_proofs,
    const std::vector<std::string>& proofs) {
  braveledger_bat_helper::Ballots ballots = ledger_->GetBallots();

  for (size_t i = 0; i < batch_proofs.size(); i++) {
    for (size_t j = 0; j < ballots.size(); j++) {
      if (ballots[j].surveyorId_ == batch_proofs[i].ballot_.surveyorId_) {
        ballots[j].proofBallot_ = proofs[i];
      }
    }
  }

  ledger_->SetBallots(ballots);

  if (batch_proofs.size() != proofs.size()) {
    contribution_->AddRetry(ledger::ContributionRetry::STEP_PROOF, "");
    return;
  }

  contribution_->SetTimer(&last_prepare_vote_batch_timer_id_);
}

void PhaseTwo::PrepareVoteBatch() {
  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();
  braveledger_bat_helper::Ballots ballots = ledger_->GetBallots();
  braveledger_bat_helper::BatchVotes batch = ledger_->GetBatch();

  if (ballots.size() == 0) {
    contribution_->SetTimer(&last_vote_batch_timer_id_);
    return;
  }

  for (int i = ballots.size() - 1; i >= 0; i--) {
    if (ballots[i].prepareBallot_.empty() || ballots[i].proofBallot_.empty()) {
      // TODO(nejczdovc) what to do in this case
      continue;
    }

    bool transaction_exit = false;
    for (size_t k = 0; k < transactions.size(); k++) {
      if (transactions[k].viewingId_ == ballots[i].viewingId_) {
        bool ballot_exit = false;
        for (size_t j = 0; j < transactions[k].ballots_.size(); j++) {
          if (transactions[k].ballots_[j].publisher_ == ballots[i].publisher_) {
            transactions[k].ballots_[j].offset_++;
            ballot_exit = true;
            break;
          }
        }

        if (!ballot_exit) {
          braveledger_bat_helper::TRANSACTION_BALLOT_ST transactionBallot;
          transactionBallot.publisher_ = ballots[i].publisher_;
          transactionBallot.offset_++;
          transactions[k].ballots_.push_back(transactionBallot);
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
    braveledger_bat_helper::BATCH_VOTES_INFO_ST batchVotesInfoSt;
    batchVotesInfoSt.surveyorId_ = ballots[i].surveyorId_;
    batchVotesInfoSt.proof_ = ballots[i].proofBallot_;

    for (size_t k = 0; k < batch.size(); k++) {
      if (batch[k].publisher_ == ballots[i].publisher_) {
        exist_batch = true;
        batch[k].batchVotesInfo_.push_back(batchVotesInfoSt);
      }
    }

    if (!exist_batch) {
      braveledger_bat_helper::BATCH_VOTES_ST batchVotesSt;
      batchVotesSt.publisher_ = ballots[i].publisher_;
      batchVotesSt.batchVotesInfo_.push_back(batchVotesInfoSt);
      batch.push_back(batchVotesSt);
    }

    ballots.erase(ballots.begin() + i);
  }

  ledger_->SetTransactions(transactions);
  ledger_->SetBallots(ballots);
  ledger_->SetBatch(batch);
  contribution_->SetTimer(&last_vote_batch_timer_id_);
}

void PhaseTwo::VoteBatch() {
  braveledger_bat_helper::BatchVotes batch = ledger_->GetBatch();
  if (batch.size() == 0) {
    return;
  }

  braveledger_bat_helper::BATCH_VOTES_ST batch_votes = batch[0];
  std::vector<braveledger_bat_helper::BATCH_VOTES_INFO_ST> vote_batch;

  if (batch_votes.batchVotesInfo_.size() > VOTE_BATCH_SIZE) {
    vote_batch.assign(batch_votes.batchVotesInfo_.begin(),
                     batch_votes.batchVotesInfo_.begin() + VOTE_BATCH_SIZE);
  } else {
    vote_batch = batch_votes.batchVotesInfo_;
  }

  std::string payload = braveledger_bat_helper::stringifyBatch(vote_batch);

  std::string url = braveledger_bat_helper::buildURL(
      (std::string)SURVEYOR_BATCH_VOTING ,
      PREFIX_V2);
  auto callback = std::bind(&PhaseTwo::VoteBatchCallback,
                            this,
                            batch_votes.publisher_,
                            _1,
                            _2,
                            _3);
  ledger_->LoadURL(url,
      std::vector<std::string>(),
      payload,
      "application/json; charset=utf-8",
      ledger::URL_METHOD::POST,
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

  braveledger_bat_helper::BatchVotes batch = ledger_->GetBatch();

  for (size_t i = 0; i < batch.size(); i++) {
    if (batch[i].publisher_ == publisher) {
      size_t sizeToCheck = VOTE_BATCH_SIZE;
      if (batch[i].batchVotesInfo_.size() < VOTE_BATCH_SIZE) {
        sizeToCheck = batch[i].batchVotesInfo_.size();
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

          if (surveyor_id == batch[i].batchVotesInfo_[j].surveyorId_) {
            batch[i].batchVotesInfo_.erase(
                batch[i].batchVotesInfo_.begin() + j);
            break;
          }
        }
      }

      if (batch[i].batchVotesInfo_.size() == 0) {
        batch.erase(batch.begin() + i);
      }
      break;
    }
  }

  ledger_->SetBatch(batch);

  if (batch.size() > 0) {
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
