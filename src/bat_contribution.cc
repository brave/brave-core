/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>
#include <map>
#include <cmath>
#include <ctime>

#include "anon/anon.h"
#include "bat_contribution.h"
#include "ledger_impl.h"
#include "rapidjson_bat_helper.h"

namespace braveledger_bat_contribution {

static bool winners_votes_compare(
    const braveledger_bat_helper::WINNERS_ST& first,
    const braveledger_bat_helper::WINNERS_ST& second){
  return (first.votes_ < second.votes_);
}

BatContribution::BatContribution(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    last_reconcile_timer_id_(0u),
    last_prepare_vote_batch_timer_id_(0u),
    last_vote_batch_timer_id_(0u) {
  initAnonize();
}

BatContribution::~BatContribution() {
}

// TODO(nejczdovc) we have the same function in bat-client
// Maybe create anonize helper?
std::string BatContribution::GetAnonizeProof(
    const std::string& registrar_VK,
    const std::string& id,
    std::string& pre_flight) {
  const char* cred = makeCred(id.c_str());
  if (nullptr != cred) {
    pre_flight = cred;
    free((void*)cred);
  } else {
    return "";
  }
  const char* proofTemp = registerUserMessage(pre_flight.c_str(),
                                              registrar_VK.c_str());
  std::string proof;
  if (nullptr != proofTemp) {
    proof = proofTemp;
    free((void*)proofTemp);
  } else {
    return "";
  }

  return proof;
}

void BatContribution::ReconcilePublisherList(
    ledger::PUBLISHER_CATEGORY category,
    const ledger::PublisherInfoList& list,
    uint32_t next_record) {
  braveledger_bat_helper::PublisherList newList;
  for (const auto &publisher : list) {
    braveledger_bat_helper::PUBLISHER_ST new_publisher;
    new_publisher.id_ = publisher.id;
    new_publisher.percent_ = publisher.percent;
    new_publisher.weight_ = publisher.weight;
    new_publisher.duration_ = publisher.duration;
    new_publisher.score_ = publisher.score;
    new_publisher.visits_ = publisher.visits;
    newList.push_back(new_publisher);
  }

  Reconcile(ledger_->GenerateGUID(), category, newList);
}

void BatContribution::OnTimerReconcile() {
  ledger_->GetRecurringDonations(
      std::bind(&BatContribution::ReconcilePublisherList,
                this,
                ledger::PUBLISHER_CATEGORY::RECURRING_DONATION,
                std::placeholders::_1,
                std::placeholders::_2));
}

void BatContribution::StartAutoContribute() {
  uint64_t currentReconcileStamp = ledger_->GetReconcileStamp();
  ledger::PublisherInfoFilter filter = ledger_->CreatePublisherFilter(
      "",
      ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
      ledger::PUBLISHER_MONTH::ANY,
      -1,
      ledger::PUBLISHER_EXCLUDE_FILTER::FILTER_DEFAULT,
      true,
      currentReconcileStamp);
  ledger_->GetCurrentPublisherInfoList(
      0,
      0,
      filter,
      std::bind(&BatContribution::ReconcilePublisherList,
                this,
                ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
                std::placeholders::_1,
                std::placeholders::_2));
}

void BatContribution::Reconcile(
    const std::string& viewingId,
    const ledger::PUBLISHER_CATEGORY category,
    const braveledger_bat_helper::PublisherList& list,
    const braveledger_bat_helper::Directions& directions) {
  if (ledger_->ReconcileExists(viewingId)) {
    ledger_->Log(__func__,
                 ledger::LogLevel::LOG_ERROR,
                 {"unable to reconcile with the same viewing id"});
    // TODO(nejczdovc) add error callback
    return;
  }

  auto reconcile = braveledger_bat_helper::CURRENT_RECONCILE();

  double fee = .0;

  double balance = ledger_->GetBalance();

  if (category == ledger::PUBLISHER_CATEGORY ::AUTO_CONTRIBUTE) {
    double ac_amount = ledger_->GetContributionAmount();

    if (list.size() == 0 || ac_amount > balance) {
      if (list.size() == 0) {
        ledger_->Log(__func__,
                     ledger::LogLevel::LOG_INFO,
                     {"AC table is empty"});
      }

      if (ac_amount > balance) {
        ledger_->Log(__func__,
                     ledger::LogLevel::LOG_INFO,
                     {"You don't have enough funds for AC contribution"});
      }

      ledger_->ResetReconcileStamp();
      // TODO(nejczdovc) add error callback
      return;
    }

    reconcile.list_ = list;
  }

  if (category == ledger::PUBLISHER_CATEGORY::RECURRING_DONATION) {
    double ac_amount = ledger_->GetContributionAmount();
    if (list.size() == 0) {
      ledger_->Log(__func__,
                   ledger::LogLevel::LOG_INFO,
                   {"recurring donation list is empty"});
      StartAutoContribute();
      // TODO(nejczdovc) add error callback
      return;
    }

    for (const auto& publisher : list) {
      if (publisher.id_.empty()) {
        ledger_->Log(__func__,
                     ledger::LogLevel::LOG_ERROR,
                     {"recurring donation is missing publisher"});
        StartAutoContribute();
        // TODO(nejczdovc) add error callback
        return;
      }

      fee += publisher.weight_;
    }

    if (fee + ac_amount > balance) {
        ledger_->Log(__func__,
                     ledger::LogLevel::LOG_ERROR,
                     {"You don't have enough funds to "
                      "do recurring and AC contribution"});
      // TODO(nejczdovc) add error callback
      return;
    }

    reconcile.list_ = list;
  }

  if (category == ledger::PUBLISHER_CATEGORY::DIRECT_DONATION) {
    for (const auto& direction : directions) {
      if (direction.publisher_key_.empty()) {
        ledger_->Log(__func__,
                     ledger::LogLevel::LOG_ERROR,
                     {"reconcile direction missing publisher"});
        // TODO(nejczdovc) add error callback
        return;
      }

      if (direction.currency_ != CURRENCY) {
        ledger_->Log(__func__,
                     ledger::LogLevel::LOG_ERROR,
                     {"reconcile direction currency invalid for ",
                      direction.publisher_key_});
        // TODO(nejczdovc) add error callback
        return;
      }

      fee += direction.amount_;
    }

    if (fee > balance) {
      ledger_->Log(__func__,
                   ledger::LogLevel::LOG_ERROR,
                   {"You don't have enough funds to do a tip"});
      // TODO(nejczdovc) add error callback
      return;
    }
  }

  reconcile.viewingId_ = viewingId;
  reconcile.fee_ = fee;
  reconcile.directions_ = directions;
  reconcile.category_ = category;

  ledger_->AddReconcile(viewingId, reconcile);

  std::string url = braveledger_bat_helper::buildURL(
      (std::string)RECONCILE_CONTRIBUTION + ledger_->GetUserId(), PREFIX_V2);
  auto request_id = ledger_->LoadURL(url,
      std::vector<std::string>(),
      "",
      "",
      ledger::URL_METHOD::GET,
      &handler_);

  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatContribution::ReconcileCallback,
                                       this,
                                       viewingId,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3));
}

void BatContribution::ReconcileCallback(
    const std::string& viewingId,
    bool result,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  auto reconcile = ledger_->GetReconcileById(viewingId);

  if (!result || reconcile.viewingId_.empty()) {
    // TODO(nejczdovc) errors handling
    return;
  }

  braveledger_bat_helper::getJSONValue(SURVEYOR_ID,
                                       response,
                                       reconcile.surveyorInfo_.surveyorId_);
  bool success = ledger_->UpdateReconcile(reconcile);
  if (!success) {
    // TODO(nejczdovc) error handling
    return;
  }

  CurrentReconcile(viewingId);
}

void BatContribution::CurrentReconcile(const std::string& viewingId) {
  std::ostringstream amount;
  auto reconcile = ledger_->GetReconcileById(viewingId);

  if (reconcile.category_ == ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE) {
    amount << ledger_->GetContributionAmount();
  } else {
    amount << reconcile.fee_;
  }

  std::string currency = ledger_->GetCurrency();
  std::string path = (std::string)WALLET_PROPERTIES +
                      ledger_->GetPaymentId() +
                      "?refresh=true" +
                      "&amount=" +
                      amount.str() +
                      "&altcurrency=" +
                      currency;

  auto request_id = ledger_->LoadURL(
      braveledger_bat_helper::buildURL(path, PREFIX_V2),
      std::vector<std::string>(),
      "",
      "",
      ledger::URL_METHOD::GET,
      &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(
                                 &BatContribution::CurrentReconcileCallback,
                                 this,
                                 viewingId,
                                 std::placeholders::_1,
                                 std::placeholders::_2,
                                 std::placeholders::_3));
}

void BatContribution::CurrentReconcileCallback(
    const std::string& viewingId,
    bool result,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  if (!result) {
    OnReconcileComplete(ledger::Result::LEDGER_ERROR, viewingId);
    // TODO(nejczdovc) errors handling
    return;
  }

  auto reconcile = ledger_->GetReconcileById(viewingId);

  braveledger_bat_helper::getJSONRates(response, reconcile.rates_);
  braveledger_bat_helper::UNSIGNED_TX unsignedTx;
  braveledger_bat_helper::getJSONUnsignedTx(response, unsignedTx);

  if (unsignedTx.amount_.empty() &&
      unsignedTx.currency_.empty() &&
      unsignedTx.destination_.empty()) {
    OnReconcileComplete(
        ledger::Result::LEDGER_ERROR, reconcile.viewingId_);
    // We don't have any unsigned transactions
    // TODO(nejczdovc) error handling
    return;
  }

  reconcile.amount_ = unsignedTx.amount_;
  reconcile.currency_ = unsignedTx.currency_;
  bool success = ledger_->UpdateReconcile(reconcile);

  if (!success) {
    // TODO(nejczdovc) error handling
    return;
  }

  braveledger_bat_helper::WALLET_INFO_ST wallet_info = ledger_->GetWalletInfo();
  std::string octets = braveledger_bat_helper::stringifyUnsignedTx(unsignedTx);

  std::string headerDigest = "SHA-256=" +
      braveledger_bat_helper::getBase64(
          braveledger_bat_helper::getSHA256(octets));

  std::string headerKeys[1] = {"digest"};
  std::string headerValues[1] = {headerDigest};

  std::vector<uint8_t> secretKey = braveledger_bat_helper::getHKDF(
      wallet_info.keyInfoSeed_);
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> newSecretKey;
  braveledger_bat_helper::getPublicKeyFromSeed(secretKey,
                                               publicKey,
                                               newSecretKey);
  std::string headerSignature = braveledger_bat_helper::sign(headerKeys,
                                                             headerValues,
                                                             1,
                                                             "primary",
                                                             newSecretKey);

  braveledger_bat_helper::RECONCILE_PAYLOAD_ST reconcilePayload;
  reconcilePayload.requestType_ = "httpSignature";
  reconcilePayload.request_signedtx_headers_digest_ = headerDigest;
  reconcilePayload.request_signedtx_headers_signature_ = headerSignature;
  reconcilePayload.request_signedtx_body_ = unsignedTx;
  reconcilePayload.request_signedtx_octets_ = octets;
  reconcilePayload.request_viewingId_ = reconcile.viewingId_;
  reconcilePayload.request_surveyorId_ = reconcile.surveyorInfo_.surveyorId_;
  std::string payloadStringify =
      braveledger_bat_helper::stringifyReconcilePayloadSt(reconcilePayload);

  std::vector<std::string> walletHeader;
  walletHeader.push_back("Content-Type: application/json; charset=UTF-8");
  std::string path = (std::string)WALLET_PROPERTIES + ledger_->GetPaymentId();

  auto request_id = ledger_->LoadURL(
      braveledger_bat_helper::buildURL(path, PREFIX_V2),
      walletHeader,
      payloadStringify,
      "application/json; charset=utf-8",
      ledger::URL_METHOD::PUT,
      &handler_);
  handler_.AddRequestHandler(
      std::move(request_id),
      std::bind(&BatContribution::ReconcilePayloadCallback,
               this,
               viewingId,
               std::placeholders::_1,
               std::placeholders::_2,
               std::placeholders::_3));
}

void BatContribution::ReconcilePayloadCallback(
    const std::string& viewingId,
    bool result,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  if (!result) {
    OnReconcileComplete(ledger::Result::LEDGER_ERROR, viewingId);
    // TODO(nejczdovc) errors handling
    return;
  }

  const auto reconcile = ledger_->GetReconcileById(viewingId);

  braveledger_bat_helper::TRANSACTION_ST transaction;
  braveledger_bat_helper::getJSONTransaction(response, transaction);
  transaction.viewingId_ = reconcile.viewingId_;
  transaction.surveyorId_ = reconcile.surveyorInfo_.surveyorId_;
  transaction.contribution_rates_ = reconcile.rates_;
  transaction.contribution_fiat_amount_ = reconcile.amount_;
  transaction.contribution_fiat_currency_ = reconcile.currency_;

  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();
  transactions.push_back(transaction);
  ledger_->SetTransactions(transactions);
  RegisterViewing(viewingId);
}

void BatContribution::RegisterViewing(const std::string& viewingId) {
  auto request_id = ledger_->LoadURL(
      braveledger_bat_helper::buildURL(
          (std::string)REGISTER_VIEWING, PREFIX_V2),
      std::vector<std::string>(),
      "",
      "",
      ledger::URL_METHOD::GET,
      &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(
                                 &BatContribution::RegisterViewingCallback,
                                 this,
                                 viewingId,
                                 std::placeholders::_1,
                                 std::placeholders::_2,
                                 std::placeholders::_3));
}

void BatContribution::RegisterViewingCallback(
    const std::string& viewingId,
    bool result,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  if (!result) {
    OnReconcileComplete(ledger::Result::LEDGER_ERROR, viewingId);
    // TODO(nejczdovc) errors handling
    return;
  }

  auto reconcile = ledger_->GetReconcileById(viewingId);

  braveledger_bat_helper::getJSONValue(REGISTRARVK_FIELDNAME,
                                       response,
                                       reconcile.registrarVK_);
  DCHECK(!reconcile.registrarVK_.empty());
  reconcile.anonizeViewingId_ = reconcile.viewingId_;
  reconcile.anonizeViewingId_.erase(
      std::remove(reconcile.anonizeViewingId_.begin(),
                  reconcile.anonizeViewingId_.end(),
                  '-'),
      reconcile.anonizeViewingId_.end());
  reconcile.anonizeViewingId_.erase(12, 1);

  std::string proof = GetAnonizeProof(reconcile.registrarVK_,
                                      reconcile.anonizeViewingId_,
                                      reconcile.preFlight_);

  bool success = ledger_->UpdateReconcile(reconcile);
  if (!success) {
    // TODO(nejczdovc) error handling
    return;
  }

  std::string keys[1] = {"proof"};
  std::string values[1] = {proof};
  std::string proofStringified = braveledger_bat_helper::stringify(keys,
                                                                   values,
                                                                   1);
  ViewingCredentials(viewingId, proofStringified, reconcile.anonizeViewingId_);
}

void BatContribution::ViewingCredentials(const std::string& viewingId,
                                         const std::string& proofStringified,
                                         const std::string& anonizeViewingId) {
  std::string url = braveledger_bat_helper::buildURL(
      (std::string)REGISTER_VIEWING +
      "/" +
      anonizeViewingId, PREFIX_V2);

  auto request_id = ledger_->LoadURL(url,
                                     std::vector<std::string>(),
                                     proofStringified,
                                     "application/json; charset=utf-8",
                                     ledger::URL_METHOD::POST,
                                     &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(
                                 &BatContribution::ViewingCredentialsCallback,
                                 this,
                                 viewingId,
                                 std::placeholders::_1,
                                 std::placeholders::_2,
                                 std::placeholders::_3));
}

void BatContribution::ViewingCredentialsCallback(
    const std::string& viewingId,
    bool result,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  if (!result) {
    OnReconcileComplete(ledger::Result::LEDGER_ERROR, viewingId);
    // TODO(nejczdovc) errors handling
    return;
  }

  auto reconcile = ledger_->GetReconcileById(viewingId);

  std::string verification;
  braveledger_bat_helper::getJSONValue(VERIFICATION_FIELDNAME,
                                       response,
                                       verification);
  const char* masterUserToken = registerUserFinal(
      reconcile.anonizeViewingId_.c_str(),
      verification.c_str(),
      reconcile.preFlight_.c_str(),
      reconcile.registrarVK_.c_str());

  if (nullptr != masterUserToken) {
    reconcile.masterUserToken_ = masterUserToken;
    free((void*)masterUserToken);
  }

  bool success = ledger_->UpdateReconcile(reconcile);
  if (!success) {
    // TODO(nejczdovc) error handling
    return;
  }

  std::vector<std::string> surveyors;
  braveledger_bat_helper::getJSONList(SURVEYOR_IDS, response, surveyors);
  std::string probi = "0";
  // Save the rest values to transactions
  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();

  for (size_t i = 0; i < transactions.size(); i++) {
    if (transactions[i].viewingId_ != reconcile.viewingId_) {
      continue;
    }

    transactions[i].anonizeViewingId_ = reconcile.anonizeViewingId_;
    transactions[i].registrarVK_ = reconcile.registrarVK_;
    transactions[i].masterUserToken_ = reconcile.masterUserToken_;
    transactions[i].surveyorIds_ = surveyors;
    probi = transactions[i].contribution_probi_;
  }

  ledger_->SetTransactions(transactions);
  OnReconcileComplete(ledger::Result::LEDGER_OK,
                               reconcile.viewingId_,
                               probi);
}

void BatContribution::OnReconcileComplete(ledger::Result result,
                                          const std::string& viewing_id,
                                          const std::string& probi) {
  // Start the timer again if it wasn't a direct donation
  auto reconcile = ledger_->GetReconcileById(viewing_id);
  if (reconcile.category_ == ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE) {
    ledger_->ResetReconcileStamp();
    SetReconcileTimer();
  }

  // Trigger auto contribute after recurring donation
  if (reconcile.category_ == ledger::PUBLISHER_CATEGORY::RECURRING_DONATION) {
    StartAutoContribute();
  }

  ledger_->OnReconcileComplete(result, viewing_id, probi);

  if (result != ledger::Result::LEDGER_OK) {
    // TODO (nejczdovc) error handling
    // TODO(nejczdovc) don't remove when we have retries
    ledger_->RemoveReconcileById(viewing_id);
    return;
  }

  unsigned int ballotsCount = GetBallotsCount(viewing_id);
  GetReconcileWinners(ballotsCount, viewing_id);
}

unsigned int BatContribution::GetBallotsCount(const std::string& viewingId) {
  unsigned int count = 0;
  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();
  for (size_t i = 0; i < transactions.size(); i++) {
    if (transactions[i].votes_ < transactions[i].surveyorIds_.size()
        && transactions[i].viewingId_ == viewingId) {
      count += transactions[i].surveyorIds_.size() - transactions[i].votes_;
    }
  }

  return count;
}

void BatContribution::GetReconcileWinners(const unsigned int& ballots,
                                              const std::string& viewing_id) {
  const auto reconcile = ledger_->GetReconcileById(viewing_id);

  switch (reconcile.category_) {
    case ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE:
      GetContributeWinners(ballots, viewing_id, reconcile.list_);
      break;

    case ledger::PUBLISHER_CATEGORY::RECURRING_DONATION:
      GetDonationWinners(ballots, viewing_id, reconcile.list_);
      break;

    case ledger::PUBLISHER_CATEGORY::DIRECT_DONATION:
      // Direct one-time contribution
      braveledger_bat_helper::WINNERS_ST winner;
      winner.votes_ = ballots;
      winner.publisher_data_.id_ = reconcile.directions_.front().publisher_key_;
      winner.publisher_data_.duration_ = 0;
      winner.publisher_data_.score_ = 0;
      winner.publisher_data_.visits_ = 0;
      winner.publisher_data_.percent_ = 0;
      winner.publisher_data_.weight_ = 0;
      VotePublishers(braveledger_bat_helper::Winners { winner },
                     viewing_id);
      break;

  }
}

void BatContribution::GetContributeWinners(
    const unsigned int& ballots,
    const std::string& viewing_id,
    const braveledger_bat_helper::PublisherList& list) {
  ledger::PublisherInfoList newList;
  ledger_->NormalizeContributeWinners(&newList, false, list, 0);
  std::sort(newList.begin(), newList.end());

  unsigned int totalVotes = 0;
  std::vector<unsigned int> votes;
  braveledger_bat_helper::Winners res;
  // TODO there is underscore.shuffle
  for (auto &item : newList) {
    if (item.percent <= 0) {
      continue;
    }

    braveledger_bat_helper::WINNERS_ST winner;
    winner.votes_ = (unsigned int)std::lround(
        (double) item.percent * (double)ballots / 100.0);

    totalVotes += winner.votes_;
    winner.publisher_data_.id_ = item.id;
    winner.publisher_data_.duration_ = item.duration;
    winner.publisher_data_.score_ = item.score;
    winner.publisher_data_.visits_ = item.visits;
    winner.publisher_data_.percent_ = item.percent;
    winner.publisher_data_.weight_ = item.weight;
    res.push_back(winner);
  }
  if (res.size()) {
    while (totalVotes > ballots) {
      braveledger_bat_helper::Winners::iterator max =
          std::max_element(res.begin(), res.end(), winners_votes_compare);
      (max->votes_)--;
      totalVotes--;
    }
  }

  VotePublishers(res, viewing_id);
}

void BatContribution::GetDonationWinners(
    const unsigned int& ballots,
    const std::string& viewing_id,
    const braveledger_bat_helper::PublisherList& list) {
  const auto reconcile = ledger_->GetReconcileById(viewing_id);
  unsigned int totalVotes = 0;
  std::vector<unsigned int> votes;
  braveledger_bat_helper::Winners res;

  for (const auto &item : list) {
    if (item.weight_ <= 0) {
      continue;
    }

    braveledger_bat_helper::WINNERS_ST winner;
    double percent = item.weight_ / reconcile.fee_;
    winner.votes_ = (unsigned int)std::lround(percent * (double)ballots);
    totalVotes += winner.votes_;
    winner.publisher_data_.id_ = item.id_;
    winner.publisher_data_.duration_ = 0;
    winner.publisher_data_.score_ = 0;
    winner.publisher_data_.visits_ = 0;
    winner.publisher_data_.percent_ = 0;
    winner.publisher_data_.weight_ = 0;
    res.push_back(winner);
  }

  if (res.size()) {
    while (totalVotes > ballots) {
      braveledger_bat_helper::Winners::iterator max =
          std::max_element(res.begin(), res.end(), winners_votes_compare);
      (max->votes_)--;
      totalVotes--;
    }
  }

  VotePublishers(res, viewing_id);
}

void BatContribution::VotePublishers(
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

  PrepareBallots();
}

void BatContribution::VotePublisher(const std::string& publisher,
                                    const std::string& viewingId) {
  DCHECK(!publisher.empty());
  if (publisher.empty()) {
    return;
  }

  braveledger_bat_helper::BALLOT_ST ballot;
  int i = 0;

  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();
  for (i = transactions.size() - 1; i >=0; i--) {
    if (transactions[i].votes_ >= transactions[i].surveyorIds_.size()) {
      continue;
    }

    if (transactions[i].viewingId_ == viewingId || viewingId.empty()) {
      break;
    }
  }

  if (i < 0) {
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

void BatContribution::PrepareBallots() {
  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();
  braveledger_bat_helper::Ballots ballots = ledger_->GetBallots();

  for (int i = ballots.size() - 1; i >= 0; i--) {
    bool breakTheLoop = false;

    for (size_t j = 0; j < transactions.size(); j++) {
      if (transactions[j].viewingId_ == ballots[i].viewingId_) {
        if (ballots[i].prepareBallot_.empty()) {
          PrepareBatch(ballots[i], transactions[j]);
          breakTheLoop = true;
          break;
        } else {
          // TODO check on ballot.prepareBallot and
          // call commitBallot if it exist
        }
      }
    }

    if (breakTheLoop) {
      break;
    }
  }
}

void BatContribution::PrepareBatch(
    const braveledger_bat_helper::BALLOT_ST& ballot,
    const braveledger_bat_helper::TRANSACTION_ST& transaction) {
  std::string url = braveledger_bat_helper::buildURL(
      (std::string)SURVEYOR_BATCH_VOTING +
      "/" +
      transaction.anonizeViewingId_, PREFIX_V2);

  auto request_id = ledger_->LoadURL(url,
                                     std::vector<std::string>(),
                                     "",
                                     "",
                                     ledger::URL_METHOD::GET,
                                     &handler_);

  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatContribution::PrepareBatchCallback,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3));
}

void BatContribution::PrepareBatchCallback(
    bool result,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  std::vector<std::string> surveyors;
  braveledger_bat_helper::getJSONBatchSurveyors(response, surveyors);
  braveledger_bat_helper::BathProofs batchProof;

  braveledger_bat_helper::Transactions transactions =
    ledger_->GetTransactions();
  braveledger_bat_helper::Ballots ballots = ledger_->GetBallots();

  for (size_t j = 0; j < surveyors.size(); j++) {
    std::string error;
    braveledger_bat_helper::getJSONValue("error", surveyors[j], error);
    if (!error.empty()) {
      // TODO(nejczdovc) add error handler
      continue;
    }

    for (int i = ballots.size() - 1; i >= 0; i--) {
      std::string surveyor_id;
      braveledger_bat_helper::getJSONValue("surveyorId",
                                           surveyors[j],
                                           surveyor_id);

      if (ballots[i].surveyorId_ == surveyor_id) {
        for (size_t k = 0; k < transactions.size(); k++) {
          if (transactions[k].viewingId_ == ballots[i].viewingId_) {
            ballots[i].prepareBallot_ = surveyors[j];
            braveledger_bat_helper::BATCH_PROOF batchProofEl;
            batchProofEl.transaction_ = transactions[k];
            batchProofEl.ballot_ = ballots[i];
            batchProof.push_back(batchProofEl);
          }
        }
      }
    }
  }

  ledger_->SetBallots(ballots);
  ledger_->RunIOTask(std::bind(&BatContribution::ProofBatch,
                               this,
                               batchProof,
                               std::placeholders::_1));
}

void BatContribution::ProofBatch(
    const braveledger_bat_helper::BathProofs& batchProof,
    ledger::LedgerTaskRunner::CallerThreadCallback callback) {
  std::vector<std::string> proofs;

  for (size_t i = 0; i < batchProof.size(); i++) {
    braveledger_bat_helper::SURVEYOR_ST surveyor;
    bool success = braveledger_bat_helper::loadFromJson(
        surveyor,
        batchProof[i].ballot_.prepareBallot_);

    if (!success) {
      ledger_->Log(__func__,
                   ledger::LogLevel::LOG_ERROR,
                   {"Failed to load surveyor state: ",
                    batchProof[i].ballot_.prepareBallot_});
    }

    std::string signatureToSend;
    size_t delimeterPos = surveyor.signature_.find(',');
    if (std::string::npos != delimeterPos &&
        delimeterPos + 1 <= surveyor.signature_.length()) {
      signatureToSend = surveyor.signature_.substr(delimeterPos + 1);

      if (signatureToSend.length() > 1 && signatureToSend[0] == ' ') {
        signatureToSend.erase(0, 1);
      }
    }

    std::string keysMsg[1] = {"publisher"};
    std::string valuesMsg[1] = {batchProof[i].ballot_.publisher_};
    std::string msg = braveledger_bat_helper::stringify(keysMsg, valuesMsg, 1);

    const char* proof = submitMessage(
        msg.c_str(),
        batchProof[i].transaction_.masterUserToken_.c_str(),
        batchProof[i].transaction_.registrarVK_.c_str(),
        signatureToSend.c_str(),
        surveyor.surveyorId_.c_str(),
        surveyor.surveyVK_.c_str());

    std::string anonProof;
    if (nullptr != proof) {
      anonProof = proof;
      free((void*)proof);
    }

    proofs.push_back(anonProof);
  }

  callback(std::bind(&BatContribution::ProofBatchCallback, this, batchProof, proofs));
}

void BatContribution::ProofBatchCallback(
    const braveledger_bat_helper::BathProofs& batchProof,
    const std::vector<std::string>& proofs) {
  braveledger_bat_helper::Ballots ballots = ledger_->GetBallots();

  for (size_t i = 0; i < batchProof.size(); i++) {
    for (size_t j = 0; j < ballots.size(); j++) {
      if (ballots[j].surveyorId_ == batchProof[i].ballot_.surveyorId_) {
        ballots[j].proofBallot_ = proofs[i];
      }
    }
  }

  ledger_->SetBallots(ballots);
  SetTimer(last_prepare_vote_batch_timer_id_);
}

void BatContribution::PrepareVoteBatch() {
  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();
  braveledger_bat_helper::Ballots ballots = ledger_->GetBallots();
    braveledger_bat_helper::BatchVotes batch = ledger_->GetBatch();

  for (int i = ballots.size() - 1; i >= 0; i--) {
    if (ballots[i].prepareBallot_.empty() || ballots[i].proofBallot_.empty()) {
      // TODO(nejczdovc) error handling
      continue;
    }

    bool transactionExist = false;
    for (size_t k = 0; k < transactions.size(); k++) {
      if (transactions[k].viewingId_ == ballots[i].viewingId_) {
        bool existBallot = false;
        for (size_t j = 0; j < transactions[k].ballots_.size(); j++) {
          if (transactions[k].ballots_[j].publisher_ == ballots[i].publisher_) {
            transactions[k].ballots_[j].offset_++;
            existBallot = true;
            break;
          }
        }
        if (!existBallot) {
          braveledger_bat_helper::TRANSACTION_BALLOT_ST transactionBallot;
          transactionBallot.publisher_ = ballots[i].publisher_;
          transactionBallot.offset_++;
          transactions[k].ballots_.push_back(transactionBallot);
        }
        transactionExist = true;
        break;
      }
    }

    if (!transactionExist) {
      continue;
    }

    bool existBatch = false;
    braveledger_bat_helper::BATCH_VOTES_INFO_ST batchVotesInfoSt;
    batchVotesInfoSt.surveyorId_ = ballots[i].surveyorId_;
    batchVotesInfoSt.proof_ = ballots[i].proofBallot_;

    for (size_t k = 0; k < batch.size(); k++) {
      if (batch[k].publisher_ == ballots[i].publisher_) {
        existBatch = true;
        batch[k].batchVotesInfo_.push_back(batchVotesInfoSt);
      }
    }

    if (!existBatch) {
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
  SetTimer(last_vote_batch_timer_id_);
}

void BatContribution::VoteBatch() {
  braveledger_bat_helper::BatchVotes batch = ledger_->GetBatch();
  if (batch.size() == 0) {
    return;
  }

  braveledger_bat_helper::BATCH_VOTES_ST batchVotes = batch[0];
  std::vector<braveledger_bat_helper::BATCH_VOTES_INFO_ST> voteBatch;

  if (batchVotes.batchVotesInfo_.size() > VOTE_BATCH_SIZE) {
    voteBatch.assign(batchVotes.batchVotesInfo_.begin(),
                     batchVotes.batchVotesInfo_.begin() + VOTE_BATCH_SIZE);
  } else {
    voteBatch = batchVotes.batchVotesInfo_;
  }

  std::string payload = braveledger_bat_helper::stringifyBatch(voteBatch);

  std::string url = braveledger_bat_helper::buildURL(
      (std::string)SURVEYOR_BATCH_VOTING ,
      PREFIX_V2);

  auto request_id = ledger_->LoadURL(url,
                                     std::vector<std::string>(),
                                     payload,
                                     "application/json; charset=utf-8",
                                     ledger::URL_METHOD::POST,
                                     &handler_);
  handler_.AddRequestHandler(std::move(request_id),
                             std::bind(&BatContribution::VoteBatchCallback,
                                       this,
                                       batchVotes.publisher_,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3));
}

void BatContribution::VoteBatchCallback(
    const std::string& publisher,
    bool result,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, result, response, headers);

  std::vector<std::string> surveyors;
  braveledger_bat_helper::getJSONBatchSurveyors(response, surveyors);
  braveledger_bat_helper::BatchVotes batch = ledger_->GetBatch();

  for (size_t i = 0; i < batch.size(); i++) {
    if (batch[i].publisher_ == publisher) {
      size_t sizeToCheck = VOTE_BATCH_SIZE;
      if (batch[i].batchVotesInfo_.size() < VOTE_BATCH_SIZE) {
        sizeToCheck = batch[i].batchVotesInfo_.size();
      }

      for (int j = sizeToCheck - 1; j >= 0; j--) {
        for (size_t k = 0; k < surveyors.size(); k++) {
          std::string surveyorId;
          braveledger_bat_helper::getJSONValue("surveyorId",
                                               surveyors[k],
                                               surveyorId);
          if (surveyorId == batch[i].batchVotesInfo_[j].surveyorId_) {
            batch[i].batchVotesInfo_.erase(
                batch[i].batchVotesInfo_.begin() + j);
            break;
          }
        }
      }

      if (0 == batch[i].batchVotesInfo_.size()) {
        batch.erase(batch.begin() + i);
      }
      break;
    }
  }
  ledger_->SetBatch(batch);
  SetTimer(last_vote_batch_timer_id_);
}

void BatContribution::OnTimer(uint32_t timer_id) {
  if (timer_id == last_reconcile_timer_id_) {
    last_reconcile_timer_id_ = 0;
    OnTimerReconcile();
  } else if (timer_id == last_vote_batch_timer_id_) {
    last_vote_batch_timer_id_ = 0;
    VoteBatch();
  } else if (timer_id == last_prepare_vote_batch_timer_id_) {
    last_prepare_vote_batch_timer_id_ = 0;
    PrepareVoteBatch();
  }
}

void BatContribution::SetReconcileTimer() {
  if (last_reconcile_timer_id_ != 0) {
    return;
  }

  uint64_t now = std::time(nullptr);
  uint64_t nextReconcileTimestamp = ledger_->GetReconcileStamp();

  uint64_t time_to_next_reconcile =
      (nextReconcileTimestamp == 0 || nextReconcileTimestamp < now) ?
        0 : nextReconcileTimestamp - now;

  SetTimer(last_reconcile_timer_id_, time_to_next_reconcile);
}

void BatContribution::SetTimer(uint32_t timer_id, uint64_t start_timer_in) {
  if (start_timer_in == 0) {
    start_timer_in = braveledger_bat_helper::getRandomValue(10, 60);
  }

  ledger_->Log(__func__,
               ledger::LogLevel::LOG_INFO,
               {"Starts in ", std::to_string(start_timer_in)});

  ledger_->SetTimer(start_timer_in, timer_id);
}

void BatContribution::OnReconcileCompleteSuccess(
    const std::string& viewing_id,
    ledger::PUBLISHER_CATEGORY category,
    const std::string& probi,
    ledger::PUBLISHER_MONTH month,
    int year,
    uint32_t date) {
  if (category == ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE) {
    ledger_->SetBalanceReportItem(month,
                                  year,
                                  ledger::ReportType::AUTO_CONTRIBUTION,
                                  probi);
    ledger_->SaveContributionInfo(probi, month, year, date, "", category);
    return;
  }

  if (category == ledger::PUBLISHER_CATEGORY::DIRECT_DONATION) {
    ledger_->SetBalanceReportItem(month,
                                  year,
                                  ledger::ReportType::DONATION,
                                  probi);
    auto reconcile = ledger_->GetReconcileById(viewing_id);
    auto donations = reconcile.directions_;
    if (donations.size() > 0) {
      std::string publisher_key = donations[0].publisher_key_;
      ledger_->SaveContributionInfo(probi,
                                    month,
                                    year,
                                    date,
                                    publisher_key,
                                    category);
    }
    return;
  }

  if (category == ledger::PUBLISHER_CATEGORY::RECURRING_DONATION) {
    auto reconcile = ledger_->GetReconcileById(viewing_id);
    ledger_->SetBalanceReportItem(month,
                                  year,
                                  ledger::ReportType::DONATION_RECURRING,
                                  probi);
    for (auto &publisher : reconcile.list_) {
      // TODO(nejczdovc) remove when we completely switch to probi
      const std::string probi =
          std::to_string(static_cast<int>(publisher.weight_)) +
          "000000000000000000";
      ledger_->SaveContributionInfo(probi,
                                    month,
                                    year,
                                    date,
                                    publisher.id_,
                                    category);
    }
    return;
  }
}

}  // namespace braveledger_bat_contribution