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

#include "base/guid.h"
#include "base/time/time.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bat_util.h"
#include "bat/ledger/internal/common/bind_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/contribution/contribution.h"
#include "bat/ledger/internal/contribution/contribution_unblinded.h"
#include "bat/ledger/internal/contribution/contribution_util.h"
#include "bat/ledger/internal/contribution/unverified.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/internal/wallet/balance.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "brave_base/random.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {
ledger::ContributionStep ConvertResultIntoContributionStep(
    const ledger::Result result) {
  switch (result) {
    case ledger::Result::LEDGER_OK: {
      return ledger::ContributionStep::STEP_COMPLETED;
    }
    case ledger::Result::AC_TABLE_EMPTY: {
      return ledger::ContributionStep::STEP_AC_TABLE_EMPTY;
    }
    case ledger::Result::NOT_ENOUGH_FUNDS: {
      return ledger::ContributionStep::STEP_NOT_ENOUGH_FUNDS;
    }
    default: {
      return ledger::ContributionStep::STEP_FAILED;
    }
  }
}

double GetTotalFromRecurringVerified(
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

bool HaveReconcileEnoughFunds(
    ledger::ContributionQueuePtr contribution,
    double* fee,
    const double balance) {
  if (contribution->type == ledger::RewardsType::AUTO_CONTRIBUTE) {
    if (balance == 0) {
      return false;
    }

    if (contribution->amount > balance) {
      contribution->amount = balance;
    }

    *fee = contribution->amount;
    return true;
  }

  if (contribution->amount > balance) {
    return false;
  }

  *fee = contribution->amount;
  return true;
}

}  // namespace

namespace braveledger_contribution {

Contribution::Contribution(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    unverified_(std::make_unique<Unverified>(ledger, this)),
    unblinded_(std::make_unique<Unblinded>(ledger)),
    uphold_(std::make_unique<braveledger_uphold::Uphold>(ledger)),
    last_reconcile_timer_id_(0u),
    queue_timer_id_(0u) {
}

Contribution::~Contribution() = default;

void Contribution::Initialize() {
  uphold_->Initialize();
  unblinded_->Initialize();

  // Resume in progress contributions
  ledger::CurrentReconciles currentReconciles = ledger_->GetCurrentReconciles();

  for (const auto& value : currentReconciles) {
    ledger::CurrentReconcileProperties reconcile = value.second;

    if (reconcile.retry_step == ledger::ContributionRetry::STEP_FINAL ||
        reconcile.retry_step == ledger::ContributionRetry::STEP_NO) {
      ledger_->RemoveReconcileById(reconcile.viewing_id);
    } else {
      DoRetry(reconcile.viewing_id);
    }
  }

  // Process contribution queue
  CheckContributionQueue();
}

void Contribution::CheckContributionQueue() {
  const auto start_timer_in = ledger::is_testing
      ? 1
      : brave_base::random::Geometric(15);

  SetTimer(&queue_timer_id_, start_timer_in);
}

void Contribution::ProcessContributionQueue() {
  if (queue_in_progress_) {
    return;
  }

  const auto callback = std::bind(&Contribution::OnProcessContributionQueue,
      this,
      _1);
  ledger_->GetFirstContributionQueue(callback);
}

void Contribution::OnProcessContributionQueue(
    ledger::ContributionQueuePtr info) {
  if (!info) {
    queue_in_progress_ = false;
    return;
  }

  queue_in_progress_ = true;
  InitReconcile(std::move(info));
}

// TODO rename to something that tells you that you are checking
//  monthly recurring sufficient balance
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
    const ledger::Result result,
    ledger::BalancePtr properties,
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  if (result != ledger::Result::LEDGER_OK || !properties) {
    return;
  }

  auto tips_callback =
      std::bind(&Contribution::OnHasSufficientBalance,
          this,
          _1,
          properties->total,
          callback);

  ledger_->GetRecurringTips(tips_callback);
}

void Contribution::OnHasSufficientBalance(
    const ledger::PublisherInfoList& publisher_list,
    const double balance,
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  if (publisher_list.empty()) {
    callback(true);
    return;
  }

  const auto total = GetTotalFromRecurringVerified(publisher_list);
  callback(balance >= total);
}

ledger::PublisherInfoList Contribution::GetVerifiedListRecurring(
    const ledger::PublisherInfoList& list) {
  ledger::PublisherInfoList verified;
  ledger::PendingContributionList non_verified;

  for (const auto& publisher : list) {
    if (publisher->id.empty() || publisher->weight == 0.0) {
      continue;
    }

    if (publisher->status != ledger::PublisherStatus::NOT_VERIFIED) {
      verified.push_back(publisher->Clone());
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
    auto save_callback = std::bind(&Contribution::OnSavePendingContribution,
        this,
        _1);
    ledger_->SavePendingContribution(std::move(non_verified), save_callback);
  }

  return verified;
}

void Contribution::OnSavePendingContribution(const ledger::Result result) {
  ledger_->PendingContributionSaved(result);
}

void Contribution::StartRecurringTips(ledger::ResultCallback callback) {
  ledger_->GetRecurringTips(
      std::bind(&Contribution::PrepareRecurringList,
                this,
                _1,
                callback));
}

void Contribution::PrepareRecurringList(
    ledger::PublisherInfoList list,
    ledger::ResultCallback callback) {
  auto verified_list = GetVerifiedListRecurring(list);

  for (const auto &item : verified_list) {
    ledger::ContributionQueuePublisherList queue_list;
    auto publisher = ledger::ContributionQueuePublisher::New();
    publisher->publisher_key = item->id;
    publisher->amount_percent = 100.0;
    queue_list.push_back(std::move(publisher));

    auto queue = ledger::ContributionQueue::New();
    queue->type = ledger::RewardsType::RECURRING_TIP;
    queue->amount = item->weight;
    queue->partial = false;
    queue->publishers = std::move(queue_list);

    // TODO this should be batch insert and
    // callback should then do what is left after this point
    ledger_->SaveContributionQueue(
        std::move(queue),
        [](const ledger::Result _){});
  }

  CheckContributionQueue();
  callback(ledger::Result::LEDGER_OK);
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
  BLOG(ledger_, ledger::LogLevel::LOG_INFO) << "Staring monthly contribution";

  auto callback = std::bind(&Contribution::OnStartRecurringTips,
      this,
      _1);

  StartRecurringTips(callback);
}

void Contribution::OnStartRecurringTips(const ledger::Result result) {
  StartAutoContribute(ledger_->GetReconcileStamp());
  ResetReconcileStamp();
}

bool Contribution::ShouldStartAutoContribute() {
  if (!ledger_->GetRewardsMainEnabled()) {
    return false;
  }

  return ledger_->GetAutoContribute();
}

void Contribution::StartAutoContribute(uint64_t reconcile_stamp) {
  if (!ShouldStartAutoContribute()) {
    return;
  }

  BLOG(ledger_, ledger::LogLevel::LOG_INFO) << "Staring auto contribution";
  auto filter = ledger_->CreateActivityFilter(
      "",
      ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED,
      true,
      reconcile_stamp,
      false,
      ledger_->GetPublisherMinVisits());

  ledger_->GetActivityInfoList(
      0,
      0,
      std::move(filter),
      std::bind(&Contribution::PrepareACList,
                this,
                _1));
}

void Contribution::PrepareACList(ledger::PublisherInfoList list) {
  ledger::PublisherInfoList normalized_list;

  ledger_->NormalizeContributeWinners(&normalized_list, &list, 0);

  if (normalized_list.empty()) {
    return;
  }

  ledger::ContributionQueuePublisherList queue_list;
  for (const auto &item : normalized_list) {
    if (item->percent == 0) {
      continue;
    }

    auto publisher = ledger::ContributionQueuePublisher::New();
    publisher->publisher_key = item->id;
    publisher->amount_percent =  item->weight;
    queue_list.push_back(std::move(publisher));
  }

  auto queue = ledger::ContributionQueue::New();
  queue->type = ledger::RewardsType::AUTO_CONTRIBUTE;
  queue->amount = ledger_->GetContributionAmount();
  queue->partial = true;
  queue->publishers = std::move(queue_list);
  // TODO add real callback
  ledger_->SaveContributionQueue(
      std::move(queue),
      [](const ledger::Result _){});
  CheckContributionQueue();
}

void Contribution::OnBalanceForReconcile(
    const std::string& contribution_queue,
    const ledger::Result result,
    ledger::BalancePtr info) {
  auto const contribution =
      braveledger_bind_util::FromStringToContributionQueue(contribution_queue);
  if (result != ledger::Result::LEDGER_OK || !info) {
    queue_in_progress_ = false;
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
         "We couldn't get balance from the server.";
    phase_one_->Complete(ledger::Result::LEDGER_ERROR,
                         "",
                         contribution->type);
    return;
  }

  ProcessReconcile(contribution->Clone(), std::move(info));
}


void Contribution::InitReconcile(ledger::ContributionQueuePtr info) {
  const auto info_converted =
      braveledger_bind_util::FromContributionQueueToString(std::move(info));
  ledger_->FetchBalance(
      std::bind(&Contribution::OnBalanceForReconcile,
                this,
                info_converted,
                _1,
                _2));
}

void Contribution::OnTimer(uint32_t timer_id) {
  phase_two_->OnTimer(timer_id);
  unverified_->OnTimer(timer_id);
  uphold_->OnTimer(timer_id);
  unblinded_->OnTimer(timer_id);

  if (timer_id == last_reconcile_timer_id_) {
    last_reconcile_timer_id_ = 0;
    StartMonthlyContribution();
    return;
  }

  if (timer_id == queue_timer_id_) {
    ProcessContributionQueue();
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
    << "Timer will start in "
    << start_timer_in;

  ledger_->SetTimer(start_timer_in, timer_id);
}

void Contribution::ContributionCompleted(
    const std::string& contribution_id,
    const ledger::RewardsType type,
    const double amount,
    const ledger::Result result) {
  if (result == ledger::Result::LEDGER_OK) {
    ledger_->SetBalanceReportItem(
        braveledger_time_util::GetCurrentMonth(),
        braveledger_time_util::GetCurrentYear(),
        GetReportTypeFromRewardsType(type),
        amount);
  }

  ledger_->UpdateContributionInfoStepAndCount(
      contribution_id,
      ConvertResultIntoContributionStep(result),
      -1,
      [](const ledger::Result _){});
}

void Contribution::ContributeUnverifiedPublishers() {
  unverified_->Contribute();
}

void Contribution::OneTimeTip(
    const std::string& publisher_key,
    const double amount,
    ledger::ResultCallback callback) {
  if (publisher_key.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Failed to do tip due to missing publisher key";
    callback(ledger::Result::NOT_FOUND);
    return;
  }

  const auto server_callback =
    std::bind(&Contribution::OneTimeTipServerPublisher,
        this,
        _1,
        publisher_key,
        amount,
        callback);

  ledger_->GetServerPublisherInfo(publisher_key, server_callback);
}

void Contribution::SavePendingContribution(
    const std::string& publisher_key,
    double amount,
    const ledger::RewardsType type,
    ledger::ResultCallback callback) {
  auto contribution = ledger::PendingContribution::New();
  contribution->publisher_key = publisher_key;
  contribution->amount = amount;
  contribution->type = type;

  ledger::PendingContributionList list;
  list.push_back(std::move(contribution));

  ledger_->SavePendingContribution(std::move(list), callback);
}

void Contribution::OneTimeTipServerPublisher(
    ledger::ServerPublisherInfoPtr server_info,
    const std::string& publisher_key,
    double amount,
    ledger::ResultCallback callback) {
  auto status = ledger::PublisherStatus::NOT_VERIFIED;
  if (server_info) {
    status =  server_info->status;
  }

  // Save to the pending list if not verified
  if (status == ledger::PublisherStatus::NOT_VERIFIED) {
    auto save_callback = std::bind(&Contribution::OnSavePendingOneTimeTip,
        this,
        _1,
        callback);
    SavePendingContribution(
        publisher_key,
        amount,
        ledger::RewardsType::ONE_TIME_TIP,
        save_callback);
    return;
  }

  ledger::ContributionQueuePublisherList queue_list;
  auto publisher = ledger::ContributionQueuePublisher::New();
  publisher->publisher_key = publisher_key;
  publisher->amount_percent = 100.0;
  queue_list.push_back(std::move(publisher));

  auto queue = ledger::ContributionQueue::New();
  queue->type = ledger::RewardsType::ONE_TIME_TIP;
  queue->amount = amount;
  queue->partial = false;
  queue->publishers = std::move(queue_list);

  InitReconcile(std::move(queue));
  callback(ledger::Result::LEDGER_OK);
}

void Contribution::OnSavePendingOneTimeTip(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  ledger_->PendingContributionSaved(result);
  callback(result);
}

void Contribution::OnDeleteContributionQueue(const ledger::Result result) {
  queue_in_progress_ = false;
  CheckContributionQueue();
}

void Contribution::DeleteContributionQueue(const uint64_t id) {
  if (id == 0) {
    return;
  }

  auto callback = std::bind(&Contribution::OnDeleteContributionQueue,
      this,
      _1);

  ledger_->DeleteContributionQueue(id, callback);
}

bool Contribution::ProcessReconcileUnblindedTokens(
    ledger::BalancePtr info,
    ledger::RewardsType type,
    double* fee,
    ledger::ReconcileDirections directions,
    ledger::ReconcileDirections* leftovers) {
  if (!fee) {
    return false;
  }

  const double balance =
      braveledger_wallet::Balance::GetPerWalletBalance(
          ledger::kWalletUnBlinded,
          info->wallets);
  if (balance == 0) {
    return false;
  }

  const std::string contribution_id = base::GenerateGUID();

  const uint64_t now = static_cast<uint64_t>(base::Time::Now().ToDoubleT());
  auto contribution = ledger::ContributionInfo::New();
  contribution->contribution_id = contribution_id;
  contribution->amount = *fee;
  contribution->type = type;
  contribution->step = ledger::ContributionStep::STEP_START;
  contribution->retry_count = -1;
  contribution->created_at = now;
  contribution->processor = ledger::ContributionProcessor::BRAVE_TOKENS;

  ledger::ReconcileDirections new_directions;
  bool full_amount = true;
  if (balance < *fee) {
    *fee = *fee - balance;
    contribution->amount = balance;
    full_amount = false;

    if (type == ledger::RewardsType::RECURRING_TIP ||
        type == ledger::RewardsType::ONE_TIME_TIP) {
      AdjustTipsAmounts(
          directions,
          &new_directions,
          leftovers,
          balance);
    } else {
      new_directions = directions;
    }
  } else {
    new_directions = directions;
  }

  ledger::ContributionPublisherList publisher_list;
  for (auto& item : new_directions) {
    auto publisher = ledger::ContributionPublisher::New();
    publisher->contribution_id = contribution_id;
    publisher->publisher_key = item.publisher_key;
    publisher->total_amount =
        (item.amount_percent * contribution->amount) / 100;
    publisher->contributed_amount = 0;
    publisher_list.push_back(std::move(publisher));
  }

  contribution->publishers = std::move(publisher_list);
  ledger_->SaveContributionInfo(
      std::move(contribution),
      [](const ledger::Result){});
  unblinded_->Start(contribution_id);

  return full_amount;
}

bool Contribution::ProcessReconcileAnonize(
    ledger::BalancePtr info,
    ledger::RewardsType type,
    double* fee,
    ledger::ReconcileDirections directions,
    ledger::ReconcileDirections* leftovers) {
  if (!fee) {
    return false;
  }

  auto reconcile = ledger::CurrentReconcileProperties();
  reconcile.viewing_id = base::GenerateGUID();
  reconcile.fee = *fee;
  reconcile.directions = directions;
  reconcile.type = type;

  if (ledger_->ReconcileExists(reconcile.viewing_id)) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
      << "Unable to reconcile with the same viewing id: "
      << reconcile.viewing_id;
    return false;
  }

  const double balance = braveledger_wallet::Balance::GetPerWalletBalance(
      ledger::kWalletAnonymous,
      info->wallets);
  if (balance == 0) {
    return false;
  }

  if (balance >= *fee) {
    ledger_->AddReconcile(reconcile.viewing_id, reconcile);
    phase_one_->Start(reconcile.viewing_id);
    return true;
  }

  *fee = *fee - balance;
  reconcile.fee = balance;

  if (type == ledger::RewardsType::RECURRING_TIP ||
      type == ledger::RewardsType::ONE_TIME_TIP) {
    ledger::ReconcileDirections new_direction;
    AdjustTipsAmounts(directions,
                      &new_direction,
                      leftovers,
                      balance);
    reconcile.directions = new_direction;
  }

  ledger_->AddReconcile(reconcile.viewing_id, reconcile);
  phase_one_->Start(reconcile.viewing_id);
  return false;
}

bool Contribution::ProcessExternalWallet(
    ledger::BalancePtr info,
    ledger::RewardsType type,
    const double fee,
    const ledger::ReconcileDirections& directions) {
  const double balance =
      braveledger_wallet::Balance::GetPerWalletBalance(
          ledger::kWalletUphold,
          info->wallets);
  if (balance == 0) {
    return false;
  }

  const std::string contribution_id = base::GenerateGUID();

  const uint64_t now = static_cast<uint64_t>(base::Time::Now().ToDoubleT());
  auto contribution = ledger::ContributionInfo::New();
  contribution->contribution_id = contribution_id;
  contribution->amount = fee;
  contribution->type = type;
  contribution->step = ledger::ContributionStep::STEP_START;
  contribution->retry_count = -1;
  contribution->created_at = now;
  // We should change this to NONE and update it in next phase
  // when we add more external processors
  contribution->processor = ledger::ContributionProcessor::UPHOLD;

  ledger::ContributionPublisherList publisher_list;
  for (auto& item : directions) {
    auto publisher = ledger::ContributionPublisher::New();
    publisher->contribution_id = contribution_id;
    publisher->publisher_key = item.publisher_key;
    publisher->total_amount =
        (item.amount_percent * contribution->amount) / 100;
    publisher->contributed_amount = 0;
    publisher_list.push_back(std::move(publisher));
  }

  contribution->publishers = std::move(publisher_list);

  if (type == ledger::RewardsType::AUTO_CONTRIBUTE) {
    auto reconcile = ledger::CurrentReconcileProperties();
    reconcile.viewing_id = contribution_id;
    reconcile.fee = fee;
    reconcile.directions = directions;
    reconcile.type = contribution->type;
    ledger_->AddReconcile(reconcile.viewing_id, reconcile);
  }

  auto save_callback = std::bind(&Contribution::OnProcessExternalWalletSaved,
      this,
      _1,
      contribution_id,
      info->wallets);

  ledger_->SaveContributionInfo(std::move(contribution), save_callback);
  return true;
}

void Contribution::OnProcessExternalWalletSaved(
    const ledger::Result result,
    const std::string& contribution_id,
    base::flat_map<std::string, double> wallet_balances) {
  auto wallets_callback = std::bind(&Contribution::OnExternalWallets,
      this,
      contribution_id,
      wallet_balances,
      _1);

  // Check if we have token
  ledger_->GetExternalWallets(wallets_callback);
}

void Contribution::ProcessReconcile(
    ledger::ContributionQueuePtr contribution,
    ledger::BalancePtr info) {
  if (!contribution) {
    return;
  }

  if (contribution->amount == 0 || contribution->publishers.empty()) {
    DeleteContributionQueue(contribution->Clone());
    return;
  }

  double fee = .0;
  const auto have_enough_balance = HaveReconcileEnoughFunds(
      contribution->Clone(),
      &fee,
      info->total);

  if (!have_enough_balance) {
    DeleteContributionQueue(contribution->id);
    return;
  }

  if (contribution->amount == 0 || contribution->publishers.empty()) {
    DeleteContributionQueue(contribution->id);
    return;
  }

  const auto directions = FromContributionQueuePublishersToReconcileDirections(
      std::move(contribution->publishers));

  // UNBLINDED TOKENS
  ledger::ReconcileDirections anon_directions = directions;
  bool result = ProcessReconcileUnblindedTokens(
      info->Clone(),
      contribution->type,
      &fee,
      directions,
      &anon_directions);
  if (result) {
    // contribution was processed in full
    DeleteContributionQueue(contribution->id);
    return;
  }

  // USER FUNDS in ANON WALLET
  ledger::ReconcileDirections wallet_directions = anon_directions;
  result = ProcessReconcileAnonize(
      info->Clone(),
      contribution->type,
      &fee,
      anon_directions,
      &wallet_directions);
  if (result) {
    // contribution was processed in full
    DeleteContributionQueue(contribution->id);
    return;
  }

  // EXTERNAL WALLET
  result = ProcessExternalWallet(
      info->Clone(),
      contribution->type,
      fee,
      wallet_directions);

  if (result) {
    // contribution was processed in full
    DeleteContributionQueue(contribution->id);
  }
}

void Contribution::AdjustTipsAmounts(
    ledger::ReconcileDirections original_directions,
    ledger::ReconcileDirections* primary_directions,
    ledger::ReconcileDirections* rest_directions,
    double reduce_fee_for) {
  if (!primary_directions || !rest_directions) {
    return;
  }

  for (auto item : original_directions) {
    if (reduce_fee_for == 0) {
      rest_directions->push_back(item);
      continue;
    }

    if (item.amount_percent <= reduce_fee_for) {
      primary_directions->push_back(item);
      reduce_fee_for -= item.amount_percent;
      continue;
    }

    if (item.amount_percent > reduce_fee_for) {
      // primary wallet
      const auto original_weight = item.amount_percent;
      item.amount_percent = reduce_fee_for;
      primary_directions->push_back(item);

      // second wallet
      item.amount_percent = original_weight - reduce_fee_for;
      rest_directions->push_back(item);

      reduce_fee_for = 0;
    }
  }
}

void Contribution::OnExternalWallets(
    const std::string& contribution_id,
    base::flat_map<std::string, double> wallet_balances,
    std::map<std::string, ledger::ExternalWalletPtr> wallets) {
  if (wallets.size() == 0) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "No external wallets";
    ledger_->UpdateContributionInfoStepAndCount(
        contribution_id,
        ledger::ContributionStep::STEP_FAILED,
        -1,
        [](const ledger::Result _){});
    return;
  }

  ledger::ExternalWalletPtr wallet =
      braveledger_uphold::GetWallet(std::move(wallets));

  ledger_->GetContributionInfo(contribution_id,
      std::bind(&Contribution::ExternalWalletContributionInfo,
                this,
                _1,
                wallet_balances,
                *wallet));
}

void Contribution::ExternalWalletContributionInfo(
    ledger::ContributionInfoPtr contribution,
    base::flat_map<std::string, double> wallet_balances,
    const ledger::ExternalWallet& wallet) {
  // In this phase we only support one wallet
  // so we will just always pick uphold.
  // In the future we will allow user to pick which wallet to use via UI
  // and then we will extend this function
  const double uphold_balance =
      braveledger_wallet::Balance::GetPerWalletBalance(
          ledger::kWalletUphold,
          wallet_balances);

  auto result = ledger::Result::LEDGER_OK;
  if (uphold_balance < contribution->amount) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
    << "Not enough funds in uphold wallet";
    result = ledger::Result::NOT_ENOUGH_FUNDS;
  }

  if (wallet.token.empty() ||
      wallet.status != ledger::WalletStatus::VERIFIED) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
    << "Wallet token is empty/wallet is not verified " << wallet.status;
    result = ledger::Result::LEDGER_ERROR;
  }

  if (result != ledger::Result::LEDGER_OK) {
    ledger_->ContributionCompleted(
        result,
        contribution->amount,
        contribution->contribution_id,
        contribution->type);
    return;
  }

  if (contribution->type == ledger::RewardsType::AUTO_CONTRIBUTE) {
    auto callback = std::bind(&Contribution::OnUpholdAC,
                              this,
                              _1,
                              _2,
                              contribution->contribution_id);
    uphold_->TransferFunds(
        contribution->amount,
        ledger_->GetCardIdAddress(),
        ledger::ExternalWallet::New(wallet),
        callback);
    return;
  }

  for (const auto& publisher : contribution->publishers) {
    auto callback =
        std::bind(&Contribution::OnExternalWalletServerPublisherInfo,
          this,
          _1,
          contribution->contribution_id,
          publisher->total_amount,
          wallet,
          contribution->type);

    ledger_->GetServerPublisherInfo(publisher->publisher_key, callback);
  }
}

void Contribution::OnExternalWalletServerPublisherInfo(
    ledger::ServerPublisherInfoPtr info,
    const std::string& contribution_id,
    double amount,
    const ledger::ExternalWallet& wallet,
    const ledger::RewardsType type) {
  if (!info) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Publisher not found";
    ledger_->ContributionCompleted(
        ledger::Result::LEDGER_ERROR,
        amount,
        contribution_id,
        type);
    return;
  }

  if (info->status != ledger::PublisherStatus::VERIFIED) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Publisher not verified";

    auto save_callback = std::bind(&Contribution::OnSavePendingContribution,
        this,
        _1);

    SavePendingContribution(info->publisher_key, amount, type, save_callback);

    ledger_->ContributionCompleted(
        ledger::Result::LEDGER_ERROR,
        amount,
        contribution_id,
        type);
    return;
  }

  auto completed_callback = std::bind(&Contribution::ExternalWalletCompleted,
      this,
      _1,
      amount,
      contribution_id,
      type);

  uphold_->StartContribution(
      contribution_id,
      std::move(info),
      amount,
      ledger::ExternalWallet::New(wallet),
      completed_callback);
}

void Contribution::ExternalWalletCompleted(
    const ledger::Result result,
    const double amount,
    const std::string& contribution_id,
    const ledger::RewardsType type) {
  ledger_->ContributionCompleted(result, amount, contribution_id, type);
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
