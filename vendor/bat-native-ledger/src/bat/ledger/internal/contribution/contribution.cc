/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include "base/time/time.h"
#include "bat/ledger/internal/contribution/contribution.h"
#include "bat/ledger/internal/contribution/phase_one.h"
#include "bat/ledger/internal/contribution/phase_two.h"
#include "bat/ledger/internal/contribution/unverified.h"
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
    last_reconcile_timer_id_(0u) {
}

Contribution::~Contribution() {
}


void Contribution::OnStartUp() {
  // Check if we have some more pending ballots to go out
  phase_two_->PrepareBallots();

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

// static
double Contribution::GetAmountFromVerifiedAuto(
    const ledger::PublisherInfoList& publisher_list,
    double ac_amount) {
  double verified_bat = 0.0;
  for (const auto& publisher : publisher_list) {
    if (publisher->verified) {
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
    if (publisher->verified) {
      total_recurring_amount += publisher->weight;
    }
  }
  return total_recurring_amount;
}



ledger::PublisherInfoList Contribution::GetVerifiedListAuto(
    const std::string& viewing_id,
    const ledger::PublisherInfoList* list) {
  ledger::PublisherInfoList verified;
  ledger::PublisherInfoList non_verified_temp;
  ledger::PendingContributionList non_verified;

  double verified_total = 0.0;
  double verified_bat = 0.0;
  double ac_amount = ledger_->GetContributionAmount();

  for (const auto& publisher : *list) {
    if (publisher->percent == 0) {
      continue;
    }

    if (publisher->verified) {
      verified.push_back(publisher->Clone());
      verified_total += publisher->weight;
    } else {
      non_verified_temp.push_back(publisher->Clone());
    }
  }

  // verified publishers
  for (auto& publisher : verified) {
    publisher->weight = (publisher->weight / verified_total) * 100;
    publisher->percent = static_cast<uint32_t>(publisher->weight);
    *budget += publisher->weight;
  }

  // non-verified publishers
  for (const auto& publisher : non_verified_temp) {
    auto contribution = ledger::PendingContribution::New();
    contribution->amount = (publisher->weight / 100) * ac_amount;
    contribution->publisher_key = publisher->id;
    contribution->viewing_id = viewing_id;
    contribution->category = ledger::REWARDS_CATEGORY::AUTO_CONTRIBUTE;


    non_verified.push_back(std::move(contribution));
  }

  if (non_verified.size() > 0) {
    ledger_->SaveUnverifiedContribution(std::move(non_verified));
  }

  return verified;
}

ledger::PublisherInfoList Contribution::GetVerifiedListRecurring(
    const std::string& viewing_id,
    const ledger::PublisherInfoList* list,
    double* budget) {
  ledger::PublisherInfoList verified;
  ledger::PendingContributionList non_verified;

  for (const auto& publisher : *list) {
    if (publisher->id.empty() || publisher->percent == 0) {
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

void Contribution::ReconcilePublisherList(
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

void Contribution::ResetReconcileStamp() {
  ledger_->ResetReconcileStamp();
  SetReconcileTimer();
}

void Contribution::StartMonthlyContribution() {
  if (!ledger_->GetRewardsMainEnabled()) {
    ResetReconcileStamp();
    return;
  }

  ledger_->GetRecurringTips(
      std::bind(&Contribution::ReconcilePublisherList,
                this,
                ledger::REWARDS_CATEGORY::RECURRING_TIP,
                _1,
                _2));
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
      std::bind(&Contribution::ReconcilePublisherList,
                this,
                ledger::REWARDS_CATEGORY::AUTO_CONTRIBUTE,
                _1,
                _2));
}

void Contribution::OnBalanceForReconcile(
    const std::string& viewing_id,
    const ledger::REWARDS_CATEGORY category,
    const braveledger_bat_helper::PublisherList& list,
    const braveledger_bat_helper::Directions& directions,
    double budget,
    const ledger::Result result,
    ledger::BalancePtr info) {
  if (result != ledger::Result::LEDGER_OK || !info) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
         "We couldn't get balance from the server.";
    phase_one_->Complete(ledger::Result::LEDGER_ERROR,
                         viewing_id,
                         category);
    return;
  }

  phase_one_->Start(viewing_id,
                    category,
                    list,
                    directions,
                    budget,
                    info->total);
}

void Contribution::InitReconcile(
    const std::string& viewing_id,
    const ledger::REWARDS_CATEGORY category,
    const braveledger_bat_helper::PublisherList& list,
    const braveledger_bat_helper::Directions& directions,
    double budget) {
  ledger_->FetchBalance(
      std::bind(&Contribution::OnBalanceForReconcile,
                this,
                viewing_id,
                category,
                list,
                directions,
                budget,
                _1,
                _2));
}

void Contribution::OnTimer(uint32_t timer_id) {
  phase_two_->OnTimer(timer_id);
  unverified_->OnTimer(timer_id);

  if (timer_id == last_reconcile_timer_id_) {
    last_reconcile_timer_id_ = 0;
    StartMonthlyContribution();
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

void Contribution::AddRetry(
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
    phase_one_->Complete(ledger::Result::TIP_ERROR,
                         viewing_id,
                         reconcile.category_);
    return;
  }

  uint64_t start_timer_in = GetRetryTimer(step, viewing_id, &reconcile);
  bool success = ledger_->AddReconcileStep(viewing_id,
                                           reconcile.retry_step_,
                                           reconcile.retry_level_);
  if (!success || start_timer_in == 0) {
    phase_one_->Complete(ledger::Result::LEDGER_ERROR,
                         viewing_id,
                         reconcile.category_);
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
      phase_one_->Reconcile(viewing_id);
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

}  // namespace braveledger_contribution
