/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "base/time/time.h"
#include "bat/ledger/internal/contribution/contribution.h"
#include "bat/ledger/internal/contribution/phase_one.h"
#include "bat/ledger/internal/contribution/phase_two.h"
#include "bat/ledger/internal/contribution/unverified.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/internal/wallet/balance.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "brave_base/random.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_contribution {

Contribution::Contribution(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    phase_one_(std::make_unique<PhaseOne>(ledger, this)),
    phase_two_(std::make_unique<PhaseTwo>(ledger, this)),
    unverified_(std::make_unique<Unverified>(ledger, this)),
    uphold_(std::make_unique<braveledger_uphold::Uphold>(ledger)),
    last_reconcile_timer_id_(0u),
    delay_ac_timer_id(0u) {
}

Contribution::~Contribution() {
}

void Contribution::Initialize() {
  phase_two_->Initialize();
  uphold_->Initialize();

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

void Contribution::HasSufficientBalance(
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  ledger_->FetchBalance(
      std::bind(&Contribution::OnSufficientBalanceWallet,
                this,
                _1,
                _2,
                callback));
}

void Contribution::OnSufficientBalanceWallet(
    ledger::Result result,
    ledger::BalancePtr properties,
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  if (result == ledger::Result::LEDGER_OK && properties) {
    auto filter = ledger_->CreateActivityFilter(
      std::string(),
      ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED,
      true,
      ledger_->GetReconcileStamp(),
      ledger_->GetPublisherAllowNonVerified(),
      ledger_->GetPublisherMinVisits());
  ledger_->GetActivityInfoList(
      0,
      0,
      std::move(filter),
      std::bind(&Contribution::GetVerifiedAutoAmount,
                this,
                _1,
                _2,
                properties->total,
                callback));
  }
}

void Contribution::GetVerifiedAutoAmount(
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
      std::bind(&Contribution::GetVerifiedRecurringAmount,
                this,
                _1,
                _2,
                balance,
                total_reconcile_amount,
                callback));
}

double Contribution::GetAmountFromVerifiedAuto(
    const ledger::PublisherInfoList& publisher_list,
    double ac_amount) {
  double verified_bat = 0.0;
  for (const auto& publisher : publisher_list) {
    const auto add = ledger_->IsPublisherConnectedOrVerified(publisher->status);
    if (add) {
      verified_bat += (publisher->weight / 100.0) * ac_amount;
    }
  }
  return verified_bat;
}

void Contribution::GetVerifiedRecurringAmount(
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
double Contribution::GetAmountFromVerifiedRecurring(
    const ledger::PublisherInfoList& publisher_list) {
  double total_recurring_amount = 0.0;
  for (const auto& publisher : publisher_list) {
    if (publisher->id.empty()) {
      continue;
    }
    if (publisher->status == ledger::PublisherStatus::VERIFIED) {
      total_recurring_amount += publisher->weight;
    }
  }
  return total_recurring_amount;
}



ledger::PublisherInfoList Contribution::GetVerifiedListAuto(
    const ledger::PublisherInfoList& list,
    double* budget) {
  ledger::PublisherInfoList verified;
  ledger::PublisherInfoList non_verified_temp;
  ledger::PendingContributionList non_verified;

  double verified_total = 0.0;
  double ac_amount = ledger_->GetContributionAmount();

  for (const auto& publisher : list) {
    if (publisher->percent == 0) {
      continue;
    }

    const auto add = ledger_->IsPublisherConnectedOrVerified(publisher->status);
    if (add) {
      verified.push_back(publisher->Clone());
      verified_total += publisher->weight;
    } else {
      non_verified_temp.push_back(publisher->Clone());
    }
  }

  // verified budget
  *budget += (verified_total / 100) * ac_amount;

  // verified publishers
  for (auto& publisher : verified) {
    publisher->weight = (publisher->weight / verified_total) * 100;
    publisher->percent = static_cast<uint32_t>(publisher->weight);
  }

  // non-verified publishers
  for (const auto& publisher : non_verified_temp) {
    auto contribution = ledger::PendingContribution::New();
    contribution->amount = (publisher->weight / 100) * ac_amount;
    contribution->publisher_key = publisher->id;
    contribution->viewing_id = "";
    contribution->type = ledger::RewardsType::AUTO_CONTRIBUTE;

    non_verified.push_back(std::move(contribution));
  }

  if (non_verified.size() > 0) {
    ledger_->SaveUnverifiedContribution(
      std::move(non_verified),
      [](const ledger::Result _){});
  }

  return verified;
}

ledger::PublisherInfoList Contribution::GetVerifiedListRecurring(
    const ledger::PublisherInfoList& list,
    double* budget) {
  ledger::PublisherInfoList verified;
  ledger::PendingContributionList non_verified;

  for (const auto& publisher : list) {
    if (publisher->id.empty() || publisher->weight == 0.0) {
      continue;
    }

    if (publisher->status != ledger::PublisherStatus::NOT_VERIFIED) {
      verified.push_back(publisher->Clone());
      *budget += publisher->weight;
    } else {
      auto contribution = ledger::PendingContribution::New();
      contribution->amount = publisher->weight;
      contribution->publisher_key = publisher->id;
      contribution->viewing_id = "";
      contribution->type = ledger::RewardsType::RECURRING_TIP;

      non_verified.push_back(std::move(contribution));
    }
  }

  if (non_verified.size() > 0) {
    ledger_->SaveUnverifiedContribution(
      std::move(non_verified),
      [](const ledger::Result _){});
  }

  return verified;
}

void Contribution::PrepareACList(
    ledger::PublisherInfoList list,
    uint32_t next_record) {
  double budget = 0.0;
  ledger::PublisherInfoList normalized_list;
  braveledger_bat_helper::PublisherList new_list;

  ledger_->NormalizeContributeWinners(&normalized_list, &list, 0);
  auto verified_list = GetVerifiedListAuto(normalized_list, &budget);

  for (const auto &publisher : verified_list) {
    braveledger_bat_helper::PUBLISHER_ST new_publisher;
    new_publisher.id_ = publisher->id;
    new_publisher.percent_ = publisher->percent;
    new_publisher.weight_ = publisher->weight;
    new_publisher.duration_ = publisher->duration;
    new_publisher.score_ = publisher->score;
    new_publisher.visits_ = publisher->visits;
    new_publisher.status_ = static_cast<int>(publisher->status);
    new_list.push_back(new_publisher);
  }

  InitReconcile(ledger::RewardsType::AUTO_CONTRIBUTE,
                new_list,
                {},
                budget);
}

void Contribution::PrepareRecurringList(
    ledger::PublisherInfoList list,
    uint32_t next_record) {
  double budget = 0.0;

  auto verified_list = GetVerifiedListRecurring(list, &budget);
  braveledger_bat_helper::Directions directions;

  for (const auto &publisher : verified_list) {
    braveledger_bat_helper::RECONCILE_DIRECTION direction;
    direction.publisher_key_ = publisher->id;
    direction.amount_ = publisher->weight;
    direction.currency_ = "BAT";
    directions.push_back(direction);
  }

  InitReconcile(ledger::RewardsType::RECURRING_TIP,
                {},
                directions,
                budget);
}

void Contribution::ResetReconcileStamp() {
  ledger_->ResetReconcileStamp();
  SetReconcileTimer();
}

void Contribution::StartMonthlyContribution() {
    BLOG(ledger_, ledger::LogLevel::LOG_INFO) << "Staring monthly contribution";
  if (!ledger_->GetRewardsMainEnabled()) {
    ResetReconcileStamp();
    return;
  }

  ledger_->GetRecurringTips(
      std::bind(&Contribution::PrepareRecurringList,
                this,
                _1,
                _2));

  SetTimer(&delay_ac_timer_id, 10);
}

bool Contribution::ShouldStartAutoContribute() {
  if (!ledger_->GetRewardsMainEnabled()) {
    return false;
  }

  return ledger_->GetAutoContribute();
}

void Contribution::StartAutoContribute() {
  if (!ShouldStartAutoContribute()) {
    ResetReconcileStamp();
    return;
  }

  uint64_t current_reconcile_stamp = ledger_->GetReconcileStamp();
  auto filter = ledger_->CreateActivityFilter(
      "",
      ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED,
      true,
      current_reconcile_stamp,
      ledger_->GetPublisherAllowNonVerified(),
      ledger_->GetPublisherMinVisits());
  ledger_->GetActivityInfoList(
      0,
      0,
      std::move(filter),
      std::bind(&Contribution::PrepareACList,
                this,
                _1,
                _2));
}

void Contribution::OnBalanceForReconcile(
    const ledger::RewardsType type,
    const braveledger_bat_helper::PublisherList& list,
    const braveledger_bat_helper::Directions& directions,
    double budget,
    const ledger::Result result,
    ledger::BalancePtr info) {
  if (result != ledger::Result::LEDGER_OK || !info) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
         "We couldn't get balance from the server.";
    phase_one_->Complete(ledger::Result::LEDGER_ERROR,
                         "",
                         type);
    return;
  }

  ProcessReconcile(type,
                   list,
                   directions,
                   budget,
                   std::move(info));
}

void Contribution::InitReconcile(
    const ledger::RewardsType type,
    const braveledger_bat_helper::PublisherList& list,
    const braveledger_bat_helper::Directions& directions,
    double budget) {
  ledger_->FetchBalance(
      std::bind(&Contribution::OnBalanceForReconcile,
                this,
                type,
                list,
                directions,
                budget,
                _1,
                _2));
}

void Contribution::OnTimer(uint32_t timer_id) {
  phase_two_->OnTimer(timer_id);
  unverified_->OnTimer(timer_id);
  uphold_->OnTimer(timer_id);

  if (timer_id == last_reconcile_timer_id_) {
    last_reconcile_timer_id_ = 0;
    StartMonthlyContribution();
    return;
  }

  if (timer_id == delay_ac_timer_id) {
    delay_ac_timer_id = 0;
    StartAutoContribute();
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

void Contribution::SetReconcileTimer() {
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

void Contribution::SetTimer(uint32_t* timer_id, uint64_t start_timer_in) {
  if (start_timer_in == 0) {
    start_timer_in = brave_base::random::Geometric(45);
  }

  BLOG(ledger_, ledger::LogLevel::LOG_INFO)
    << "Starts in "
    << start_timer_in;

  ledger_->SetTimer(start_timer_in, timer_id);
}

void Contribution::OnReconcileCompleteSuccess(
    const std::string& viewing_id,
    const ledger::RewardsType type,
    const std::string& probi,
    ledger::ACTIVITY_MONTH month,
    int year,
    uint32_t date) {
  if (type == ledger::RewardsType::AUTO_CONTRIBUTE) {
    ledger_->SetBalanceReportItem(month,
                                  year,
                                  ledger::ReportType::AUTO_CONTRIBUTION,
                                  probi);
    ledger_->SaveContributionInfo(probi, month, year, date, "", type);
    return;
  }

  if (type == ledger::RewardsType::ONE_TIME_TIP) {
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
                                    type);
    }
    return;
  }

  if (type == ledger::RewardsType::RECURRING_TIP) {
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
                                    type);
    }
    return;
  }
}

void Contribution::AddRetry(
    ledger::ContributionRetry step,
    const std::string& viewing_id,
    braveledger_bat_helper::CURRENT_RECONCILE reconcile) {
  BLOG(ledger_, ledger::LogLevel::LOG_WARNING)
      << "Re-trying contribution for step"
      << std::to_string(static_cast<int32_t>(step))
      << "for" << viewing_id;

  if (reconcile.viewingId_.empty()) {
    reconcile = ledger_->GetReconcileById(viewing_id);
  }

  // Don't retry one-time tip if in phase 1
  if (GetRetryPhase(step) == 1 &&
      reconcile.type_ == ledger::RewardsType::ONE_TIME_TIP) {
    phase_one_->Complete(ledger::Result::TIP_ERROR,
                         viewing_id,
                         reconcile.type_);
    return;
  }

  uint64_t start_timer_in = GetRetryTimer(step, viewing_id, &reconcile);
  bool success = ledger_->AddReconcileStep(viewing_id,
                                           reconcile.retry_step_,
                                           reconcile.retry_level_);
  if (!success || start_timer_in == 0) {
    phase_one_->Complete(ledger::Result::LEDGER_ERROR,
                         viewing_id,
                         reconcile.type_);
    return;
  }

  retry_timers_[viewing_id] = 0u;
  SetTimer(&retry_timers_[viewing_id], start_timer_in);
}

uint64_t Contribution::GetRetryTimer(
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

int Contribution::GetRetryPhase(ledger::ContributionRetry step) {
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

void Contribution::DoRetry(const std::string& viewing_id) {
  auto reconcile = ledger_->GetReconcileById(viewing_id);

  switch (reconcile.retry_step_) {
    case ledger::ContributionRetry::STEP_RECONCILE: {
      phase_one_->Start(viewing_id);
      break;
    }
    case ledger::ContributionRetry::STEP_CURRENT: {
      phase_one_->CurrentReconcile(viewing_id);
      break;
    }
    case ledger::ContributionRetry::STEP_PAYLOAD: {
      phase_one_->ReconcilePayload(viewing_id);
      break;
    }
    case ledger::ContributionRetry::STEP_REGISTER: {
      phase_one_->RegisterViewing(viewing_id);
      break;
    }
    case ledger::ContributionRetry::STEP_VIEWING: {
      phase_one_->ViewingCredentials(viewing_id);
      break;
    }
    case ledger::ContributionRetry::STEP_PREPARE: {
      phase_two_->PrepareBallots();
      break;
    }
    case ledger::ContributionRetry::STEP_PROOF: {
      phase_two_->Proof();
      break;
    }
    case ledger::ContributionRetry::STEP_VOTE: {
      phase_two_->VoteBatch();
      break;
    }
    case ledger::ContributionRetry::STEP_WINNERS: {
      phase_two_->Start(viewing_id);
      break;
    }
    case ledger::ContributionRetry::STEP_FINAL:
    case ledger::ContributionRetry::STEP_NO:
      break;
  }
}

void Contribution::ContributeUnverifiedPublishers() {
  unverified_->Contribute();
}

void Contribution::StartPhaseTwo(const std::string& viewing_id) {
  phase_two_->Start(viewing_id);
}

void Contribution::DoDirectTip(
    const std::string& publisher_key,
    int amount,
    const std::string& currency,
    ledger::DoDirectTipCallback callback) {
  if (publisher_key.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Failed direct donation due to missing publisher id";
    callback(ledger::Result::NOT_FOUND);
    return;
  }

  const auto server_callback =
    std::bind(&Contribution::OnDoDirectTipServerPublisher,
              this,
              _1,
              publisher_key,
              amount,
              currency,
              callback);

  ledger_->GetServerPublisherInfo(publisher_key, server_callback);
}

void Contribution::SavePendingContribution(
    const std::string& publisher_key,
    double amount,
    const ledger::RewardsType type,
    ledger::SavePendingContributionCallback callback) {
  auto contribution = ledger::PendingContribution::New();
  contribution->publisher_key = publisher_key;
  contribution->amount = amount;
  contribution->type = type;

  ledger::PendingContributionList list;
  list.push_back(std::move(contribution));

  ledger_->SaveUnverifiedContribution(
      std::move(list),
      callback);
}

void Contribution::OnDoDirectTipServerPublisher(
    ledger::ServerPublisherInfoPtr server_info,
    const std::string& publisher_key,
    int amount,
    const std::string& currency,
    ledger::DoDirectTipCallback callback) {
  auto status = ledger::PublisherStatus::NOT_VERIFIED;
  if (server_info) {
    status =  server_info->status;
  }

  // Save to the pending list if not verified
  if (status == ledger::PublisherStatus::NOT_VERIFIED) {
    SavePendingContribution(
        publisher_key,
        static_cast<double>(amount),
        ledger::RewardsType::ONE_TIME_TIP,
        callback);
    return;
  }

  const auto direction = braveledger_bat_helper::RECONCILE_DIRECTION(
      publisher_key,
      amount,
      currency);
  const auto direction_list =
      std::vector<braveledger_bat_helper::RECONCILE_DIRECTION> { direction };
  InitReconcile(
      ledger::RewardsType::ONE_TIME_TIP,
      {},
      direction_list);
  callback(ledger::Result::LEDGER_OK);
}

bool Contribution::HaveReconcileEnoughFunds(
    const ledger::RewardsType type,
    double* fee,
    double budget,
    double balance,
    const braveledger_bat_helper::Directions& directions) {
  if (type == ledger::RewardsType::AUTO_CONTRIBUTE) {
    if (budget > balance) {
      BLOG(ledger_, ledger::LogLevel::LOG_WARNING) <<
          "You do not have enough funds for auto contribution";
       phase_one_->Complete(ledger::Result::NOT_ENOUGH_FUNDS,
                            "",
                            type);
      return false;
    }

    *fee = budget;
    return true;
  }

  if (type == ledger::RewardsType::RECURRING_TIP) {
    double ac_amount = ledger_->GetContributionAmount();

    // don't use ac amount if ac is disabled
    if (!ShouldStartAutoContribute()) {
      ac_amount = 0;
    }

    if (budget + ac_amount > balance) {
      BLOG(ledger_, ledger::LogLevel::LOG_WARNING) <<
        "You do not have enough funds to do recurring and auto contribution";
        phase_one_->Complete(ledger::Result::NOT_ENOUGH_FUNDS,
                             "",
                             ledger::RewardsType::AUTO_CONTRIBUTE);
      return false;
    }

    *fee = budget;
    return true;
  }

  if (type == ledger::RewardsType::ONE_TIME_TIP) {
    for (const auto& direction : directions) {
      if (direction.publisher_key_.empty()) {
        BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
          "Reconcile direction missing publisher";
        phase_one_->Complete(ledger::Result::TIP_ERROR,
                             "",
                             type);
        return false;
      }

      if (direction.currency_ != LEDGER_CURRENCY || direction.amount_ == 0) {
        BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
          "Reconcile direction currency invalid for " <<
          direction.publisher_key_;
        phase_one_->Complete(ledger::Result::TIP_ERROR,
                             "",
                             type);
        return false;
      }

      *fee += direction.amount_;
    }

    if (*fee > balance) {
      BLOG(ledger_, ledger::LogLevel::LOG_WARNING) <<
        "You do not have enough funds to do a tip";
        phase_one_->Complete(ledger::Result::NOT_ENOUGH_FUNDS,
                             "",
                             type);
      return false;
    }

    return true;
  }

  return false;
}

bool Contribution::IsListEmpty(
    const ledger::RewardsType type,
    const braveledger_bat_helper::PublisherList& list,
    const braveledger_bat_helper::Directions& directions,
    double budget) {
  if (type == ledger::RewardsType::AUTO_CONTRIBUTE) {
    if (list.size() == 0 || budget == 0) {
      BLOG(ledger_, ledger::LogLevel::LOG_INFO) <<
        "Auto contribution table is empty";
      phase_one_->Complete(ledger::Result::AC_TABLE_EMPTY,
                           "",
                           type);
      return true;
    }
  }

  if (type == ledger::RewardsType::RECURRING_TIP) {
    if (directions.size() == 0 || budget == 0) {
      phase_one_->Complete(ledger::Result::RECURRING_TABLE_EMPTY,
                           "",
                           ledger::RewardsType::RECURRING_TIP);
      BLOG(ledger_, ledger::LogLevel::LOG_INFO) <<
        "Recurring tips list is empty";
      return true;
    }
  }

  return false;
}

void Contribution::ProcessReconcile(
    const ledger::RewardsType type,
    const braveledger_bat_helper::PublisherList& list,
    const braveledger_bat_helper::Directions& directions,
    double budget,
    ledger::BalancePtr info) {
  double fee = .0;
  const auto have_enough_balance = HaveReconcileEnoughFunds(type,
                                                            &fee,
                                                            budget,
                                                            info->total,
                                                            directions);
  if (!have_enough_balance) {
    return;
  }

  if (IsListEmpty(type, list, directions, budget)) {
    return;
  }

  auto anon_reconcile = braveledger_bat_helper::CURRENT_RECONCILE();
  anon_reconcile.viewingId_ = ledger_->GenerateGUID();
  anon_reconcile.fee_ = fee;
  anon_reconcile.directions_ = directions;
  anon_reconcile.type_ = type;
  anon_reconcile.list_ = list;

  if (ledger_->ReconcileExists(anon_reconcile.viewingId_)) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
      << "Unable to reconcile with the same viewing id: "
      << anon_reconcile.viewingId_;
    return;
  }

  // Check if we can process contribution with anon wallet only
  const double anon_balance = braveledger_wallet::Balance::GetPerWalletBalance(
      ledger::kWalletAnonymous,
      info->wallets);
  if (anon_balance >= fee) {
    ledger_->AddReconcile(anon_reconcile.viewingId_, anon_reconcile);
    phase_one_->Start(anon_reconcile.viewingId_);
    return;
  }

  // We need to first use all anon balance and what is left should
  // go through connected wallet
  braveledger_bat_helper::Directions wallet_directions;
  if (anon_balance > 0) {
    fee = fee - anon_balance;
    anon_reconcile.fee_ = anon_balance;

    if (type == ledger::RewardsType::RECURRING_TIP ||
        type == ledger::RewardsType::ONE_TIME_TIP) {
      braveledger_bat_helper::Directions anon_directions;
      AdjustTipsAmounts(directions,
                        &wallet_directions,
                        &anon_directions,
                        anon_balance);
      anon_reconcile.directions_ = anon_directions;
    }

    ledger_->AddReconcile(anon_reconcile.viewingId_, anon_reconcile);
    phase_one_->Start(anon_reconcile.viewingId_);
  } else {
    wallet_directions = directions;
  }

  auto wallet_reconcile = braveledger_bat_helper::CURRENT_RECONCILE();
  wallet_reconcile.viewingId_ = ledger_->GenerateGUID();
  wallet_reconcile.fee_ = fee;
  wallet_reconcile.directions_ = wallet_directions;
  wallet_reconcile.type_ = type;
  wallet_reconcile.list_ = list;
  ledger_->AddReconcile(wallet_reconcile.viewingId_, wallet_reconcile);

  auto tokens_callback = std::bind(&Contribution::OnExternalWallets,
                                   this,
                                   wallet_reconcile.viewingId_,
                                   info->wallets,
                                   _1);

  // Check if we have token
  ledger_->GetExternalWallets(tokens_callback);
}

void Contribution::AdjustTipsAmounts(
    braveledger_bat_helper::Directions directions,
    braveledger_bat_helper::Directions* wallet_directions,
    braveledger_bat_helper::Directions* anon_directions,
    double reduce_fee_for) {
  for (auto item : directions) {
    if (reduce_fee_for == 0) {
      wallet_directions->push_back(item);
      continue;
    }

    if (item.amount_ <= reduce_fee_for) {
      anon_directions->push_back(item);
      reduce_fee_for -= item.amount_;
      continue;
    }

    if (item.amount_ > reduce_fee_for) {
      // anon wallet
      const auto original_weight = item.amount_;
      item.amount_ = reduce_fee_for;
      anon_directions->push_back(item);

      // rest to normal wallet
      item.amount_ = original_weight - reduce_fee_for;
      wallet_directions->push_back(item);

      reduce_fee_for = 0;
    }
  }
}

void Contribution::OnExternalWallets(
    const std::string& viewing_id,
    base::flat_map<std::string, double> wallet_balances,
    std::map<std::string, ledger::ExternalWalletPtr> wallets) {
  // In this phase we only support one wallet
  // so we will just always pick uphold.
  // In the future we will allow user to pick which wallet to use via UI
  // and then we will extend this function
  const double uphold_balance =
      braveledger_wallet::Balance::GetPerWalletBalance(ledger::kWalletUphold,
                                                       wallet_balances);
  const auto reconcile = ledger_->GetReconcileById(viewing_id);

  if (wallets.size() == 0 || uphold_balance < reconcile.fee_) {
    phase_one_->Complete(ledger::Result::NOT_ENOUGH_FUNDS,
                         viewing_id,
                         reconcile.type_);
    return;
  }

  ledger::ExternalWalletPtr wallet =
      braveledger_uphold::GetWallet(std::move(wallets));
  if (!wallet || wallet->token.empty()) {
    phase_one_->Complete(ledger::Result::LEDGER_ERROR,
                         viewing_id,
                         reconcile.type_);
    return;
  }

  if (reconcile.type_ == ledger::RewardsType::AUTO_CONTRIBUTE) {
    auto callback = std::bind(&Contribution::OnUpholdAC,
                              this,
                              _1,
                              _2,
                              viewing_id);
    uphold_->TransferFunds(reconcile.fee_,
                           ledger_->GetCardIdAddress(),
                           std::move(wallet),
                           callback);
    return;
  }

  for (const auto& item : reconcile.directions_) {
    auto callback =
        std::bind(&Contribution::OnExternalWalletServerPublisherInfo,
          this,
          _1,
          viewing_id,
          item.amount_,
          *wallet);

    ledger_->GetServerPublisherInfo(item.publisher_key_, callback);
  }
}

void Contribution::OnExternalWalletServerPublisherInfo(
    ledger::ServerPublisherInfoPtr info,
    const std::string& viewing_id,
    int amount,
    const ledger::ExternalWallet& wallet) {
  const auto reconcile = ledger_->GetReconcileById(viewing_id);
  if (!info) {
    const auto probi =
        braveledger_uphold::ConvertToProbi(std::to_string(amount));
    ledger_->OnReconcileComplete(
        ledger::Result::LEDGER_ERROR,
        viewing_id,
        probi,
        reconcile.type_);

    if (!viewing_id.empty()) {
      ledger_->RemoveReconcileById(viewing_id);
    }
    return;
  }

  if (info->status != ledger::PublisherStatus::VERIFIED) {
    SavePendingContribution(
        info->publisher_key,
        static_cast<double>(amount),
        static_cast<ledger::RewardsType>(reconcile.type_),
        [](const ledger::Result _){});
    return;
  }

  uphold_->StartContribution(
      viewing_id,
      info->address,
      static_cast<double>(amount),
      ledger::ExternalWallet::New(wallet));
}

void Contribution::OnUpholdAC(ledger::Result result,
                              bool created,
                              const std::string& viewing_id) {
  if (result != ledger::Result::LEDGER_OK) {
    // TODO(nejczdovc): add retries
    return;
  }

  phase_one_->Start(viewing_id);
}

}  // namespace braveledger_contribution
