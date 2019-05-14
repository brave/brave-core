/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <map>
#include <memory>
#include <vector>
#include <utility>

#include "anon/anon.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "bat/ledger/internal/bat_contribution.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "net/http/http_status_code.h"
#include "brave_base/random.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_bat_contribution {

static bool winners_votes_compare(
    const braveledger_bat_helper::WINNERS_ST& first,
    const braveledger_bat_helper::WINNERS_ST& second) {
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


void BatContribution::OnStartUp() {
  // Check if we have some more pending ballots to go out
  PrepareBallots();

  // Resume in progress contributions
  braveledger_bat_helper::CurrentReconciles currentReconciles =
      ledger_->GetCurrentReconciles();

  for (const auto& value : currentReconciles) {
    braveledger_bat_helper::CURRENT_RECONCILE reconcile = value.second;

    if (reconcile.retry_step_ == ledger::ContributionRetry::STEP_FINAL) {
      ledger_->RemoveReconcileById(reconcile.viewingId_);
    } else {
      DoRetry(reconcile.viewingId_);
    }
  }
}

// TODO(nejczdovc) we have the same function in bat-client
// Maybe create anon helper?
std::string BatContribution::GetAnonizeProof(
    const std::string& registrar_VK,
    const std::string& id,
    std::string* pre_flight) {
  const char* cred = makeCred(id.c_str());
  if (cred != nullptr) {
    *pre_flight = std::string(cred);
      // should fix in
      // https://github.com/brave-intl/bat-native-anonize/issues/11
      free((void*)cred); // NOLINT
  } else {
    return "";
  }
  const char* proof_temp = registerUserMessage(pre_flight->c_str(),
                                              registrar_VK.c_str());
  std::string proof;
  if (proof_temp != nullptr) {
    proof = proof_temp;
    // should fix in
    // https://github.com/brave-intl/bat-native-anonize/issues/11
    free((void*)proof_temp); // NOLINT
  } else {
    return "";
  }

  return proof;
}

void BatContribution::HasSufficientBalance(
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  ledger_->FetchWalletProperties(
      std::bind(&BatContribution::OnSufficientBalanceWallet,
        this, _1, _2, callback));
}

void BatContribution::GetVerifiedAutoAmount(
    const ledger::PublisherInfoList& publisher_list,
    uint32_t record,
    double balance,
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  double ac_amount = ledger_->GetContributionAmount();
  double total_reconcile_amount(GetAmountFromVerifiedAuto(
      publisher_list, ac_amount));
  if (balance < total_reconcile_amount && !publisher_list.empty()) {
    callback(false);
    return;
  }
  ledger_->GetRecurringTips(
      std::bind(&BatContribution::GetVerifiedRecurringAmount,
                this,
                _1,
                _2,
                balance,
                total_reconcile_amount,
                callback));
}

// static
double BatContribution::GetAmountFromVerifiedAuto(
    const ledger::PublisherInfoList& publisher_list,
    double ac_amount) {
  double non_verified_bat = 0.0;
  for (const auto& publisher : publisher_list) {
    if (!publisher->verified) {
      non_verified_bat += (publisher->percent / 100.0) * ac_amount;
    }
  }
  return ac_amount - non_verified_bat;
}

void BatContribution::GetVerifiedRecurringAmount(
    const ledger::PublisherInfoList& publisher_list,
    uint32_t record,
    double balance,
    double total_reconcile_amount,
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  if (publisher_list.empty()) {
    callback(true);
    return;
  }
  total_reconcile_amount += GetAmountFromVerifiedRecurring(publisher_list);
  callback(balance >= total_reconcile_amount);
}

// static
double BatContribution::GetAmountFromVerifiedRecurring(
    const ledger::PublisherInfoList& publisher_list) {
  double total_recurring_amount(0.0);
  for (const auto& publisher : publisher_list) {
    if (publisher->id.empty()) {
      continue;
    }
    if (publisher->verified) {
      total_recurring_amount += publisher->weight;
    }
  }
  return total_recurring_amount;
}

void BatContribution::OnSufficientBalanceWallet(
    ledger::Result result,
    std::unique_ptr<ledger::WalletInfo> info,
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  if (result == ledger::Result::LEDGER_OK && info) {
    ledger::ActivityInfoFilter filter = ledger_->CreateActivityFilter(
      std::string(),
      ledger::EXCLUDE_FILTER::FILTER_ALL_EXCEPT_EXCLUDED,
      true,
      ledger_->GetReconcileStamp(),
      ledger_->GetPublisherAllowNonVerified(),
      ledger_->GetPublisherMinVisits());
  ledger_->GetActivityInfoList(
      0,
      0,
      filter,
      std::bind(&BatContribution::GetVerifiedAutoAmount,
                this,
                _1,
                _2,
                info->balance_,
                callback));
  }
}

ledger::PublisherInfoList BatContribution::GetVerifiedListAuto(
    const std::string& viewing_id,
    const ledger::PublisherInfoList* list,
    double* budget) {
  ledger::PublisherInfoList verified;
  ledger::PublisherInfoList temp;
  ledger::PendingContributionList non_verified;

  double verified_total = 0.0;
  double non_verified_bat = 0.0;
  double ac_amount = ledger_->GetContributionAmount();

  for (const auto& publisher : *list) {
    if (publisher->verified) {
      verified.push_back(publisher->Clone());
      verified_total += publisher->percent;
    } else {
      temp.push_back(publisher->Clone());
    }
  }

  // verified publishers
  for (auto& publisher : verified) {
    publisher->percent = static_cast<uint32_t>(
        static_cast<double>(publisher->percent) / verified_total) * 100;
  }

  // non-verified publishers
  for (const auto& publisher : temp) {
    auto contribution = ledger::PendingContribution::New();
    contribution->amount =
        (static_cast<double>(publisher->percent) / 100) * ac_amount;
    contribution->publisher_key = publisher->id;
    contribution->viewing_id = viewing_id;
    contribution->category = ledger::REWARDS_CATEGORY::AUTO_CONTRIBUTE;

    non_verified_bat += contribution->amount;
    non_verified.push_back(std::move(contribution));
  }

  if (non_verified.size() > 0) {
    ledger_->SaveUnverifiedContribution(std::move(non_verified));
  }

  *budget = ac_amount - non_verified_bat;

  return verified;
}

ledger::PublisherInfoList BatContribution::GetVerifiedListRecurring(
    const std::string& viewing_id,
    const ledger::PublisherInfoList* list,
    double* budget) {
  ledger::PublisherInfoList verified;
  ledger::PendingContributionList non_verified;

  for (const auto& publisher : *list) {
    if (publisher->id.empty()) {
      continue;
    }

    if (publisher->verified) {
      verified.push_back(publisher->Clone());
      *budget += publisher->weight;
    } else {
      auto contribution = ledger::PendingContribution::New();
      contribution->amount = publisher->weight;
      contribution->publisher_key = publisher->id;
      contribution->viewing_id = viewing_id;
      contribution->category = ledger::REWARDS_CATEGORY::RECURRING_TIP;

      non_verified.push_back(std::move(contribution));
    }
  }

  if (non_verified.size() > 0) {
    ledger_->SaveUnverifiedContribution(std::move(non_verified));
  }

  return verified;
}

void BatContribution::ReconcilePublisherList(
    ledger::REWARDS_CATEGORY category,
    ledger::PublisherInfoList list,
    uint32_t next_record) {
  std::string viewing_id = ledger_->GenerateGUID();
  ledger::PublisherInfoList verified_list;
  double budget = 0.0;

  if (category == ledger::REWARDS_CATEGORY::AUTO_CONTRIBUTE) {
    ledger::PublisherInfoList normalized_list;
    ledger_->NormalizeContributeWinners(&normalized_list, &list, 0);
    verified_list = GetVerifiedListAuto(viewing_id, &normalized_list, &budget);
  } else {
    verified_list = GetVerifiedListRecurring(viewing_id, &list, &budget);
  }

  braveledger_bat_helper::PublisherList new_list;

  for (const auto &publisher : verified_list) {
    braveledger_bat_helper::PUBLISHER_ST new_publisher;
    new_publisher.id_ = publisher->id;
    new_publisher.percent_ = publisher->percent;
    new_publisher.weight_ = publisher->weight;
    new_publisher.duration_ = publisher->duration;
    new_publisher.score_ = publisher->score;
    new_publisher.visits_ = publisher->visits;
    new_publisher.verified_ = publisher->verified;
    new_list.push_back(new_publisher);
  }

  InitReconcile(viewing_id, category, new_list, {}, budget);
}

void BatContribution::ResetReconcileStamp() {
  ledger_->ResetReconcileStamp();
  SetReconcileTimer();
}

void BatContribution::OnTimerReconcile() {
  if (!ledger_->GetRewardsMainEnabled()) {
    ResetReconcileStamp();
    return;
  }

  ledger_->GetRecurringTips(
      std::bind(&BatContribution::ReconcilePublisherList,
                this,
                ledger::REWARDS_CATEGORY::RECURRING_TIP,
                _1,
                _2));
}

bool BatContribution::ShouldStartAutoContribute() {
  if (!ledger_->GetRewardsMainEnabled()) {
    return false;
  }

  return ledger_->GetAutoContribute();
}

void BatContribution::StartAutoContribute() {
  if (!ShouldStartAutoContribute()) {
    ResetReconcileStamp();
    return;
  }

  uint64_t current_reconcile_stamp = ledger_->GetReconcileStamp();
  ledger::ActivityInfoFilter filter = ledger_->CreateActivityFilter(
      "",
      ledger::EXCLUDE_FILTER::FILTER_ALL_EXCEPT_EXCLUDED,
      true,
      current_reconcile_stamp,
      ledger_->GetPublisherAllowNonVerified(),
      ledger_->GetPublisherMinVisits());
  ledger_->GetActivityInfoList(
      0,
      0,
      filter,
      std::bind(&BatContribution::ReconcilePublisherList,
                this,
                ledger::REWARDS_CATEGORY::AUTO_CONTRIBUTE,
                _1,
                _2));
}

void BatContribution::OnWalletPropertiesForReconcile(
    const std::string& viewing_id,
    const ledger::REWARDS_CATEGORY category,
    const braveledger_bat_helper::PublisherList& list,
    const braveledger_bat_helper::Directions& directions,
    double budget,
    const ledger::Result result,
    std::unique_ptr<ledger::WalletInfo> info) {
  if (result != ledger::Result::LEDGER_OK || !info) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
         "We couldn't get balance from the server.";
    OnReconcileComplete(ledger::Result::LEDGER_ERROR,
                        viewing_id,
                        category);
    return;
  }

  StartReconcile(viewing_id,
                 category,
                 list,
                 directions,
                 budget,
                 info->balance_);
}

void BatContribution::InitReconcile(
    const std::string& viewing_id,
    const ledger::REWARDS_CATEGORY category,
    const braveledger_bat_helper::PublisherList& list,
    const braveledger_bat_helper::Directions& directions,
    double budget) {
  ledger_->FetchWalletProperties(
      std::bind(&BatContribution::OnWalletPropertiesForReconcile,
                this,
                viewing_id,
                category,
                list,
                directions,
                budget,
                _1,
                _2));
}

void BatContribution::StartReconcile(
    const std::string& viewing_id,
    const ledger::REWARDS_CATEGORY category,
    const braveledger_bat_helper::PublisherList& list,
    const braveledger_bat_helper::Directions& directions,
    double budget,
    double balance) {
  if (ledger_->ReconcileExists(viewing_id)) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Unable to reconcile with the same viewing id: " << viewing_id;
    // TODO(nejczdovc) what should we do in this scenario?
    return;
  }

  auto reconcile = braveledger_bat_helper::CURRENT_RECONCILE();
  double fee = .0;

  if (category == ledger::REWARDS_CATEGORY::AUTO_CONTRIBUTE) {
    if (list.size() == 0 || budget > balance || budget == 0) {
      if (list.size() == 0 || budget == 0) {
        BLOG(ledger_, ledger::LogLevel::LOG_INFO) <<
          "Auto contribution table is empty";
        OnReconcileComplete(ledger::Result::AC_TABLE_EMPTY,
                            viewing_id,
                            category);
        return;
      }

      if (budget > balance) {
        BLOG(ledger_, ledger::LogLevel::LOG_WARNING) <<
          "You do not have enough funds for auto contribution";
        OnReconcileComplete(ledger::Result::NOT_ENOUGH_FUNDS,
                            viewing_id,
                            category);
        return;
      }
    }

    reconcile.list_ = list;
    fee = budget;
  }

  if (category == ledger::REWARDS_CATEGORY::RECURRING_TIP) {
    double ac_amount = ledger_->GetContributionAmount();

    // don't use ac amount if ac is disabled
    if (!ShouldStartAutoContribute()) {
      ac_amount = 0;
    }

    if (list.size() == 0 || budget == 0) {
      BLOG(ledger_, ledger::LogLevel::LOG_INFO) <<
        "Recurring donation list is empty";
      StartAutoContribute();
      return;
    }

    if (budget + ac_amount > balance) {
      BLOG(ledger_, ledger::LogLevel::LOG_WARNING) <<
        "You do not have enough funds to do recurring and auto contribution";
        OnReconcileComplete(ledger::Result::NOT_ENOUGH_FUNDS,
                            viewing_id,
                            ledger::REWARDS_CATEGORY::AUTO_CONTRIBUTE);
      return;
    }

    reconcile.list_ = list;
    fee = budget;
  }

  if (category == ledger::REWARDS_CATEGORY::ONE_TIME_TIP) {
    for (const auto& direction : directions) {
      if (direction.publisher_key_.empty()) {
        BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
          "Reconcile direction missing publisher";
        OnReconcileComplete(ledger::Result::TIP_ERROR,
                            viewing_id,
                            category);
        return;
      }

      if (direction.currency_ != LEDGER_CURRENCY || direction.amount_ == 0) {
        BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
          "Reconcile direction currency invalid for " <<
          direction.publisher_key_;
        OnReconcileComplete(ledger::Result::TIP_ERROR,
                            viewing_id,
                            category);
        return;
      }

      fee += direction.amount_;
    }

    if (fee > balance) {
      BLOG(ledger_, ledger::LogLevel::LOG_WARNING) <<
        "You do not have enough funds to do a tip";
        OnReconcileComplete(ledger::Result::NOT_ENOUGH_FUNDS,
                            viewing_id,
                            category);
      return;
    }
  }

  reconcile.viewingId_ = viewing_id;
  reconcile.fee_ = fee;
  reconcile.directions_ = directions;
  reconcile.category_ = category;

  ledger_->AddReconcile(viewing_id, reconcile);
  Reconcile(viewing_id);
}

void BatContribution::Reconcile(const std::string& viewing_id) {
  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_RECONCILE);
  std::string url = braveledger_bat_helper::buildURL(
      (std::string)RECONCILE_CONTRIBUTION + ledger_->GetUserId(), PREFIX_V2);

  auto callback = std::bind(&BatContribution::ReconcileCallback,
                            this,
                            viewing_id,
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

void BatContribution::ReconcileCallback(
    const std::string& viewing_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  auto reconcile = ledger_->GetReconcileById(viewing_id);

  if (response_status_code != net::HTTP_OK || reconcile.viewingId_.empty()) {
    AddRetry(ledger::ContributionRetry::STEP_RECONCILE,
             viewing_id);
    return;
  }

  bool success = braveledger_bat_helper::getJSONValue(
      SURVEYOR_ID,
      response,
      &reconcile.surveyorInfo_.surveyorId_);
  if (!success) {
    AddRetry(ledger::ContributionRetry::STEP_RECONCILE, viewing_id);
    return;
  }

  success = ledger_->UpdateReconcile(reconcile);
  if (!success) {
    OnReconcileComplete(ledger::Result::LEDGER_ERROR,
                        viewing_id,
                        reconcile.category_);
    return;
  }

  CurrentReconcile(viewing_id);
}

void BatContribution::CurrentReconcile(const std::string& viewing_id) {
  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_CURRENT);
  std::ostringstream amount;
  auto reconcile = ledger_->GetReconcileById(viewing_id);

  amount << reconcile.fee_;

  std::string currency = ledger_->GetCurrency();
  std::string path = (std::string)WALLET_PROPERTIES +
                      ledger_->GetPaymentId() +
                      "?refresh=true" +
                      "&amount=" +
                      amount.str() +
                      "&altcurrency=" +
                      currency;

  auto callback = std::bind(&BatContribution::CurrentReconcileCallback,
                            this,
                            viewing_id,
                            _1,
                            _2,
                            _3);
  ledger_->LoadURL(
      braveledger_bat_helper::buildURL(path, PREFIX_V2),
      std::vector<std::string>(),
      "",
      "",
      ledger::URL_METHOD::GET,
      callback);
}

void BatContribution::CurrentReconcileCallback(
    const std::string& viewing_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    AddRetry(ledger::ContributionRetry::STEP_CURRENT,
             viewing_id);
    return;
  }

  auto reconcile = ledger_->GetReconcileById(viewing_id);

  bool success = braveledger_bat_helper::getJSONRates(response,
                                                      &reconcile.rates_);
  if (!success) {
    AddRetry(ledger::ContributionRetry::STEP_CURRENT, viewing_id);
    return;
  }

  braveledger_bat_helper::UNSIGNED_TX unsigned_tx;
  success = braveledger_bat_helper::getJSONUnsignedTx(response, &unsigned_tx);
  if (!success) {
    AddRetry(ledger::ContributionRetry::STEP_CURRENT, viewing_id);
    return;
  }

  if (unsigned_tx.amount_.empty() &&
      unsigned_tx.currency_.empty() &&
      unsigned_tx.destination_.empty()) {
    // We don't have any unsigned transactions
    AddRetry(ledger::ContributionRetry::STEP_CURRENT, viewing_id);
    return;
  }

  reconcile.amount_ = unsigned_tx.amount_;
  reconcile.currency_ = unsigned_tx.currency_;
  reconcile.destination_ = unsigned_tx.destination_;
  success = ledger_->UpdateReconcile(reconcile);

  if (!success) {
    OnReconcileComplete(ledger::Result::LEDGER_ERROR,
                        viewing_id,
                        reconcile.category_);
    return;
  }

  ReconcilePayload(viewing_id);
}

void BatContribution::ReconcilePayload(const std::string& viewing_id) {
  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_PAYLOAD);
  auto reconcile = ledger_->GetReconcileById(viewing_id);
  braveledger_bat_helper::WALLET_INFO_ST wallet_info = ledger_->GetWalletInfo();

  braveledger_bat_helper::UNSIGNED_TX unsigned_tx;
  unsigned_tx.amount_ = reconcile.amount_;
  unsigned_tx.currency_ = reconcile.currency_;
  unsigned_tx.destination_ = reconcile.destination_;
  std::string octets = braveledger_bat_helper::stringifyUnsignedTx(unsigned_tx);

  std::string header_digest = "SHA-256=" +
      braveledger_bat_helper::getBase64(
          braveledger_bat_helper::getSHA256(octets));

  std::string header_keys[1] = {"digest"};
  std::string header_values[1] = {header_digest};

  std::vector<uint8_t> secret_key = braveledger_bat_helper::getHKDF(
      wallet_info.keyInfoSeed_);
  std::vector<uint8_t> public_key;
  std::vector<uint8_t> new_secret_key;
  bool success = braveledger_bat_helper::getPublicKeyFromSeed(
      secret_key,
      &public_key,
      &new_secret_key);
  if (!success) {
    // TODO(nejczdovc) what should we do in this case?
    return;
  }

  std::string headerSignature = braveledger_bat_helper::sign(header_keys,
                                                             header_values,
                                                             1,
                                                             "primary",
                                                             new_secret_key);

  braveledger_bat_helper::RECONCILE_PAYLOAD_ST reconcile_payload;
  reconcile_payload.requestType_ = "httpSignature";
  reconcile_payload.request_signedtx_headers_digest_ = header_digest;
  reconcile_payload.request_signedtx_headers_signature_ = headerSignature;
  reconcile_payload.request_signedtx_body_ = unsigned_tx;
  reconcile_payload.request_signedtx_octets_ = octets;
  reconcile_payload.request_viewingId_ = reconcile.viewingId_;
  reconcile_payload.request_surveyorId_ = reconcile.surveyorInfo_.surveyorId_;
  std::string payload_stringify =
      braveledger_bat_helper::stringifyReconcilePayloadSt(reconcile_payload);

  std::vector<std::string> wallet_header;
  wallet_header.push_back("Content-Type: application/json; charset=UTF-8");
  std::string path = (std::string)WALLET_PROPERTIES + ledger_->GetPaymentId();

  auto callback = std::bind(&BatContribution::ReconcilePayloadCallback,
                            this,
                            viewing_id,
                            _1,
                            _2,
                            _3);
  ledger_->LoadURL(
      braveledger_bat_helper::buildURL(path, PREFIX_V2),
      wallet_header,
      payload_stringify,
      "application/json; charset=utf-8",
      ledger::URL_METHOD::PUT,
      callback);
}

void BatContribution::ReconcilePayloadCallback(
    const std::string& viewing_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  const auto reconcile = ledger_->GetReconcileById(viewing_id);

  if (response_status_code != net::HTTP_OK) {
    if (response_status_code == net::HTTP_REQUESTED_RANGE_NOT_SATISFIABLE) {
      OnReconcileComplete(ledger::Result::CONTRIBUTION_AMOUNT_TOO_LOW,
                          viewing_id,
                          reconcile.category_);
    } else {
      AddRetry(ledger::ContributionRetry::STEP_PAYLOAD,
               viewing_id);
    }
    return;
  }

  braveledger_bat_helper::TRANSACTION_ST transaction;
  bool success = braveledger_bat_helper::getJSONTransaction(response,
                                                            &transaction);
  if (!success) {
    AddRetry(ledger::ContributionRetry::STEP_PAYLOAD, viewing_id);
    return;
  }

  transaction.viewingId_ = reconcile.viewingId_;
  transaction.surveyorId_ = reconcile.surveyorInfo_.surveyorId_;
  transaction.contribution_rates_ = reconcile.rates_;
  transaction.contribution_fiat_amount_ = reconcile.amount_;
  transaction.contribution_fiat_currency_ = reconcile.currency_;

  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();
  transactions.push_back(transaction);
  ledger_->SetTransactions(transactions);
  RegisterViewing(viewing_id);
}

void BatContribution::RegisterViewing(const std::string& viewing_id) {
  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_REGISTER);
  auto callback = std::bind(&BatContribution::RegisterViewingCallback,
                            this,
                            viewing_id,
                            _1,
                            _2,
                            _3);
  ledger_->LoadURL(
      braveledger_bat_helper::buildURL(
        (std::string)REGISTER_VIEWING, PREFIX_V2),
      std::vector<std::string>(),
      "",
      "",
      ledger::URL_METHOD::GET,
      callback);
}

void BatContribution::RegisterViewingCallback(
    const std::string& viewing_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    AddRetry(ledger::ContributionRetry::STEP_REGISTER,
             viewing_id);
    return;
  }

  auto reconcile = ledger_->GetReconcileById(viewing_id);

  bool success = braveledger_bat_helper::getJSONValue(REGISTRARVK_FIELDNAME,
                                                      response,
                                                      &reconcile.registrarVK_);
  DCHECK(!reconcile.registrarVK_.empty());
  if (!success || reconcile.registrarVK_.empty()) {
    AddRetry(ledger::ContributionRetry::STEP_REGISTER, viewing_id);
    return;
  }

  reconcile.anonizeViewingId_ = reconcile.viewingId_;
  reconcile.anonizeViewingId_.erase(
      std::remove(reconcile.anonizeViewingId_.begin(),
                  reconcile.anonizeViewingId_.end(),
                  '-'),
      reconcile.anonizeViewingId_.end());
  reconcile.anonizeViewingId_.erase(12, 1);
  reconcile.proof_ = GetAnonizeProof(reconcile.registrarVK_,
                                     reconcile.anonizeViewingId_,
                                     &reconcile.preFlight_);

  success = ledger_->UpdateReconcile(reconcile);
  if (!success) {
    OnReconcileComplete(ledger::Result::LEDGER_ERROR,
                        viewing_id,
                        reconcile.category_);
    return;
  }

  ViewingCredentials(viewing_id);
}

void BatContribution::ViewingCredentials(const std::string& viewing_id) {
  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_VIEWING);
  auto reconcile = ledger_->GetReconcileById(viewing_id);

  std::string keys[1] = {"proof"};
  std::string values[1] = {reconcile.proof_};
  std::string proof_stringified = braveledger_bat_helper::stringify(keys,
                                                                    values,
                                                                    1);

  std::string url = braveledger_bat_helper::buildURL(
      (std::string)REGISTER_VIEWING +
      "/" +
      reconcile.anonizeViewingId_,
      PREFIX_V2);

  auto callback = std::bind(&BatContribution::ViewingCredentialsCallback,
                            this,
                            viewing_id,
                            _1,
                            _2,
                            _3);
  ledger_->LoadURL(url,
      std::vector<std::string>(),
      proof_stringified,
      "application/json; charset=utf-8",
      ledger::URL_METHOD::POST,
      callback);
}

void BatContribution::ViewingCredentialsCallback(
    const std::string& viewing_id,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    AddRetry(ledger::ContributionRetry::STEP_VIEWING,
             viewing_id);
    return;
  }

  auto reconcile = ledger_->GetReconcileById(viewing_id);

  std::string verification;
  bool success = braveledger_bat_helper::getJSONValue(VERIFICATION_FIELDNAME,
                                                      response,
                                                      &verification);
  if (!success) {
    AddRetry(ledger::ContributionRetry::STEP_VIEWING, viewing_id);
    return;
  }

  const char* master_user_token = registerUserFinal(
      reconcile.anonizeViewingId_.c_str(),
      verification.c_str(),
      reconcile.preFlight_.c_str(),
      reconcile.registrarVK_.c_str());

  if (master_user_token != nullptr) {
    reconcile.masterUserToken_ = master_user_token;
    // should fix in
    // https://github.com/brave-intl/bat-native-anonize/issues/11
    free((void*)master_user_token); // NOLINT
  }

  success = ledger_->UpdateReconcile(reconcile);
  if (!success) {
    OnReconcileComplete(ledger::Result::LEDGER_ERROR,
                        viewing_id,
                        reconcile.category_);
    return;
  }

  std::vector<std::string> surveyors;
  success = braveledger_bat_helper::getJSONList(SURVEYOR_IDS,
                                                response,
                                                &surveyors);
  if (!success) {
    AddRetry(ledger::ContributionRetry::STEP_VIEWING, viewing_id);
    return;
  }

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
                      reconcile.category_,
                      probi);
}

void BatContribution::OnReconcileComplete(ledger::Result result,
                                          const std::string& viewing_id,
                                          int category,
                                          const std::string& probi) {
  // Start the timer again if it wasn't a direct tip
  if (category == ledger::REWARDS_CATEGORY::AUTO_CONTRIBUTE) {
    ResetReconcileStamp();
  }

  // Trigger auto contribute after recurring tip
  if (category == ledger::REWARDS_CATEGORY::RECURRING_TIP) {
    StartAutoContribute();
  }

  ledger_->OnReconcileComplete(result, viewing_id, probi);

  if (result != ledger::Result::LEDGER_OK) {
    ledger_->RemoveReconcileById(viewing_id);
    return;
  }

  ledger_->AddReconcileStep(viewing_id,
                            ledger::ContributionRetry::STEP_WINNERS);
  GetReconcileWinners(viewing_id);
}

unsigned int BatContribution::GetBallotsCount(const std::string& viewing_id) {
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

void BatContribution::GetReconcileWinners(const std::string& viewing_id) {
  unsigned int ballots_count = GetBallotsCount(viewing_id);
  const auto reconcile = ledger_->GetReconcileById(viewing_id);

  switch (reconcile.category_) {
    case ledger::REWARDS_CATEGORY::AUTO_CONTRIBUTE: {
      GetContributeWinners(ballots_count, viewing_id, reconcile.list_);
      break;
    }

    case ledger::REWARDS_CATEGORY::RECURRING_TIP: {
      GetTipsWinners(ballots_count, viewing_id, reconcile.list_);
      break;
    }

    case ledger::REWARDS_CATEGORY::ONE_TIME_TIP: {
      // Direct one-time contribution
      braveledger_bat_helper::WINNERS_ST winner;
      winner.votes_ = ballots_count;
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

    default:
      // TODO(nejczdovc) what should we do here?
      return;
  }
}

void BatContribution::GetContributeWinners(
    const unsigned int ballots,
    const std::string& viewing_id,
    const braveledger_bat_helper::PublisherList& list) {
  unsigned int total_votes = 0;
  std::vector<unsigned int> votes;
  braveledger_bat_helper::Winners res;

  for (auto &item : list) {
    if (item.percent_ <= 0) {
      continue;
    }

    braveledger_bat_helper::WINNERS_ST winner;
    winner.votes_ = static_cast<unsigned int>(std::lround(
        static_cast<double>(item.percent_) *
        static_cast<double>(ballots) / 100.0));

    total_votes += winner.votes_;
    winner.publisher_data_.id_ = item.id_;
    winner.publisher_data_.duration_ = item.duration_;
    winner.publisher_data_.score_ = item.score_;
    winner.publisher_data_.visits_ = item.visits_;
    winner.publisher_data_.percent_ = item.percent_;
    winner.publisher_data_.weight_ = item.weight_;
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

void BatContribution::GetTipsWinners(
    const unsigned int ballots,
    const std::string& viewing_id,
    const braveledger_bat_helper::PublisherList& list) {
  const auto reconcile = ledger_->GetReconcileById(viewing_id);
  unsigned int total_votes = 0;
  std::vector<unsigned int> votes;
  braveledger_bat_helper::Winners res;

  for (const auto &item : list) {
    if (item.weight_ <= 0) {
      continue;
    }

    braveledger_bat_helper::WINNERS_ST winner;
    double percent = item.weight_ / reconcile.fee_;
    winner.votes_ = static_cast<unsigned int>(std::lround(percent *
        static_cast<double>(ballots)));
    total_votes += winner.votes_;
    winner.publisher_data_.id_ = item.id_;
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

  ledger_->AddReconcileStep(viewing_id, ledger::ContributionRetry::STEP_FINAL);

  PrepareBallots();
}

void BatContribution::VotePublisher(const std::string& publisher,
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

void BatContribution::PrepareBallots() {
  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();
  braveledger_bat_helper::Ballots ballots = ledger_->GetBallots();

  if (ballots.size() == 0) {
    // skip ballots and start sending votes
    SetTimer(&last_vote_batch_timer_id_);
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

void BatContribution::PrepareBatch(
    const braveledger_bat_helper::BALLOT_ST& ballot,
    const braveledger_bat_helper::TRANSACTION_ST& transaction) {
  std::string url = braveledger_bat_helper::buildURL(
      (std::string)SURVEYOR_BATCH_VOTING +
      "/" +
      transaction.anonizeViewingId_, PREFIX_V2);

  auto callback = std::bind(&BatContribution::PrepareBatchCallback,
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

void BatContribution::PrepareBatchCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    AddRetry(ledger::ContributionRetry::STEP_PREPARE, "");
    return;
  }

  std::vector<std::string> surveyors;
  bool success = braveledger_bat_helper::getJSONBatchSurveyors(response,
                                                               &surveyors);
  if (!success) {
    AddRetry(ledger::ContributionRetry::STEP_PREPARE, "");
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

void BatContribution::Proof() {
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

  base::PostTaskAndReplyWithResult(
      ledger_->GetTaskRunner().get(),
      FROM_HERE,
      base::BindOnce(&BatContribution::ProofBatch,
        base::Unretained(this),
        batch_proofs),
      base::BindOnce(&BatContribution::ProofBatchCallback,
        base::Unretained(this),
        batch_proofs));
}

std::vector<std::string> BatContribution::ProofBatch(
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

void BatContribution::ProofBatchCallback(
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
    AddRetry(ledger::ContributionRetry::STEP_PROOF, "");
    return;
  }

  SetTimer(&last_prepare_vote_batch_timer_id_);
}

void BatContribution::PrepareVoteBatch() {
  braveledger_bat_helper::Transactions transactions =
      ledger_->GetTransactions();
  braveledger_bat_helper::Ballots ballots = ledger_->GetBallots();
  braveledger_bat_helper::BatchVotes batch = ledger_->GetBatch();

  if (ballots.size() == 0) {
    SetTimer(&last_vote_batch_timer_id_);
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
  SetTimer(&last_vote_batch_timer_id_);
}

void BatContribution::VoteBatch() {
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
  auto callback = std::bind(&BatContribution::VoteBatchCallback,
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

void BatContribution::VoteBatchCallback(
    const std::string& publisher,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    AddRetry(ledger::ContributionRetry::STEP_VOTE, "");
    return;
  }

  std::vector<std::string> surveyors;
  bool success = braveledger_bat_helper::getJSONBatchSurveyors(response,
                                                               &surveyors);
  if (!success) {
    AddRetry(ledger::ContributionRetry::STEP_VOTE, "");
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
    SetTimer(&last_vote_batch_timer_id_);
  }
}

void BatContribution::OnTimer(uint32_t timer_id) {
  if (timer_id == last_reconcile_timer_id_) {
    last_reconcile_timer_id_ = 0;
    OnTimerReconcile();
    return;
  }

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

  for (std::pair<std::string, uint32_t> const& value : retry_timers_) {
    if (value.second == timer_id) {
      std::string viewing_id = value.first;
      DoRetry(viewing_id);
      retry_timers_[viewing_id] = 0u;
    }
  }
}

void BatContribution::SetReconcileTimer() {
  if (last_reconcile_timer_id_ != 0) {
    return;
  }

  uint64_t now = std::time(nullptr);
  uint64_t next_reconcile_stamp = ledger_->GetReconcileStamp();

  uint64_t time_to_next_reconcile =
      (next_reconcile_stamp == 0 || next_reconcile_stamp < now) ?
        0 : next_reconcile_stamp - now;

  SetTimer(&last_reconcile_timer_id_, time_to_next_reconcile);
}

void BatContribution::SetTimer(uint32_t* timer_id, uint64_t start_timer_in) {
  if (start_timer_in == 0) {
    start_timer_in = brave_base::random::Geometric(45);
  }

  BLOG(ledger_, ledger::LogLevel::LOG_INFO) <<
    "Starts in " << start_timer_in;

  ledger_->SetTimer(start_timer_in, timer_id);
}

void BatContribution::OnReconcileCompleteSuccess(
    const std::string& viewing_id,
    ledger::REWARDS_CATEGORY category,
    const std::string& probi,
    ledger::ACTIVITY_MONTH month,
    int year,
    uint32_t date) {
  if (category == ledger::REWARDS_CATEGORY::AUTO_CONTRIBUTE) {
    ledger_->SetBalanceReportItem(month,
                                  year,
                                  ledger::ReportType::AUTO_CONTRIBUTION,
                                  probi);
    ledger_->SaveContributionInfo(probi, month, year, date, "", category);
    return;
  }

  if (category == ledger::REWARDS_CATEGORY::ONE_TIME_TIP) {
    ledger_->SetBalanceReportItem(month,
                                  year,
                                  ledger::ReportType::TIP,
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

  if (category == ledger::REWARDS_CATEGORY::RECURRING_TIP) {
    auto reconcile = ledger_->GetReconcileById(viewing_id);
    ledger_->SetBalanceReportItem(month,
                                  year,
                                  ledger::ReportType::TIP_RECURRING,
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

void BatContribution::AddRetry(
    ledger::ContributionRetry step,
    const std::string& viewing_id,
    braveledger_bat_helper::CURRENT_RECONCILE reconcile) {
  BLOG(ledger_, ledger::LogLevel::LOG_WARNING)
      << "Re-trying contribution for step"
      << std::to_string(step)
      << "for" << viewing_id;

  if (reconcile.viewingId_.empty()) {
    reconcile = ledger_->GetReconcileById(viewing_id);
  }

  // Don't retry one-time tip if in phase 1
  if (GetRetryPhase(step) == 1 &&
      reconcile.category_ == ledger::REWARDS_CATEGORY::ONE_TIME_TIP) {
    OnReconcileComplete(ledger::Result::TIP_ERROR,
                        viewing_id,
                        reconcile.category_);
    return;
  }

  uint64_t start_timer_in = GetRetryTimer(step, viewing_id, &reconcile);
  bool success = ledger_->AddReconcileStep(viewing_id,
                                           reconcile.retry_step_,
                                           reconcile.retry_level_);
  if (!success || start_timer_in == 0) {
    OnReconcileComplete(ledger::Result::LEDGER_ERROR,
                        viewing_id,
                        reconcile.category_);
    return;
  }

  retry_timers_[viewing_id] = 0u;
  SetTimer(&retry_timers_[viewing_id], start_timer_in);
}

uint64_t BatContribution::GetRetryTimer(
    ledger::ContributionRetry step,
    const std::string& viewing_id,
    braveledger_bat_helper::CURRENT_RECONCILE* reconcile) {
  ledger::ContributionRetry old_step = reconcile->retry_step_;

  int phase = GetRetryPhase(step);
  if (phase > GetRetryPhase(old_step)) {
    reconcile->retry_level_ = 0;
  } else {
    reconcile->retry_level_++;
  }

  reconcile->retry_step_ = step;

  if (phase == 1) {
    // TODO(nejczdovc) get size from the list
    if (reconcile->retry_level_ < 5) {
      if (ledger::short_retries) {
        return phase_one_debug_timers[reconcile->retry_level_];
      } else {
        return phase_one_timers[reconcile->retry_level_];
      }

    } else {
      return 0;
    }
  }

  if (phase == 2) {
    // TODO(nejczdovc) get size from the list
    if (reconcile->retry_level_ > 2) {
      if (ledger::short_retries) {
        return phase_two_debug_timers[2];
      } else {
        return phase_two_timers[2];
      }
    } else {
      if (ledger::short_retries) {
        return phase_two_debug_timers[reconcile->retry_level_];
      } else {
        return phase_two_timers[reconcile->retry_level_];
      }
    }
  }

  return 0;
}

int BatContribution::GetRetryPhase(ledger::ContributionRetry step) {
  int phase = 0;

  switch (step) {
    case ledger::ContributionRetry::STEP_RECONCILE:
    case ledger::ContributionRetry::STEP_CURRENT:
    case ledger::ContributionRetry::STEP_PAYLOAD:
    case ledger::ContributionRetry::STEP_REGISTER:
    case ledger::ContributionRetry::STEP_VIEWING: {
      phase = 1;
      break;
    }
    case ledger::ContributionRetry::STEP_PREPARE:
    case ledger::ContributionRetry::STEP_VOTE:
    case ledger::ContributionRetry::STEP_PROOF:
    case ledger::ContributionRetry::STEP_WINNERS:
    case ledger::ContributionRetry::STEP_FINAL: {
      phase = 2;
      break;
    }
    case ledger::ContributionRetry::STEP_NO:
      break;
  }

  return phase;
}

void BatContribution::DoRetry(const std::string& viewing_id) {
  auto reconcile = ledger_->GetReconcileById(viewing_id);

  switch (reconcile.retry_step_) {
    case ledger::ContributionRetry::STEP_RECONCILE: {
      Reconcile(viewing_id);
      break;
    }
    case ledger::ContributionRetry::STEP_CURRENT: {
      CurrentReconcile(viewing_id);
      break;
    }
    case ledger::ContributionRetry::STEP_PAYLOAD: {
      ReconcilePayload(viewing_id);
      break;
    }
    case ledger::ContributionRetry::STEP_REGISTER: {
      RegisterViewing(viewing_id);
      break;
    }
    case ledger::ContributionRetry::STEP_VIEWING: {
      ViewingCredentials(viewing_id);
      break;
    }
    case ledger::ContributionRetry::STEP_PREPARE: {
      PrepareBallots();
      break;
    }
    case ledger::ContributionRetry::STEP_PROOF: {
      Proof();
      break;
    }
    case ledger::ContributionRetry::STEP_VOTE: {
      VoteBatch();
      break;
    }
    case ledger::ContributionRetry::STEP_WINNERS: {
      GetReconcileWinners(viewing_id);
      break;
    }
    case ledger::ContributionRetry::STEP_FINAL:
    case ledger::ContributionRetry::STEP_NO:
      break;
  }
}

}  // namespace braveledger_bat_contribution
