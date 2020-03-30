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
#include "bat/ledger/internal/contribution/contribution_ac.h"
#include "bat/ledger/internal/contribution/contribution_anon_card.h"
#include "bat/ledger/internal/contribution/contribution_external_wallet.h"
#include "bat/ledger/internal/contribution/contribution_monthly.h"
#include "bat/ledger/internal/contribution/contribution_tip.h"
#include "bat/ledger/internal/contribution/contribution_sku.h"
#include "bat/ledger/internal/contribution/contribution_unblinded.h"
#include "bat/ledger/internal/contribution/contribution_util.h"
#include "bat/ledger/internal/contribution/unverified.h"
#include "bat/ledger/internal/uphold/uphold.h"
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

}  // namespace

namespace braveledger_contribution {

Contribution::Contribution(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    unverified_(std::make_unique<Unverified>(ledger, this)),
    unblinded_(std::make_unique<Unblinded>(ledger)),
    sku_(std::make_unique<ContributionSKU>(ledger, this)),
    uphold_(std::make_unique<braveledger_uphold::Uphold>(ledger)),
    monthly_(std::make_unique<ContributionMonthly>(ledger, this)),
    ac_(std::make_unique<ContributionAC>(ledger, this)),
    tip_(std::make_unique<ContributionTip>(ledger, this)),
    anon_card_(std::make_unique<ContributionAnonCard>(ledger, this)),
    last_reconcile_timer_id_(0u),
    queue_timer_id_(0u) {
  DCHECK(ledger_ && uphold_);
  external_wallet_ = std::make_unique<ContributionExternalWallet>(
      ledger,
      this,
      uphold_.get());
}

Contribution::~Contribution() = default;

void Contribution::Initialize() {
  uphold_->Initialize();

  CheckContributionQueue();
  CheckNotCompletedContributions();
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
  Start(std::move(info));
}

void Contribution::CheckNotCompletedContributions() {
  auto get_callback = std::bind(&Contribution::NotCompletedContributions,
      this,
      _1);

  ledger_->GetNotCompletedContributions(get_callback);
}

void Contribution::NotCompletedContributions(
    ledger::ContributionInfoList list) {
  if (list.size() == 0) {
    return;
  }

  for (auto& item : list) {
    if (!item) {
      continue;
    }

    SetRetryCounter(std::move(item));
  }
}

void Contribution::HasSufficientBalance(
    ledger::HasSufficientBalanceToReconcileCallback callback) {
  monthly_->HasSufficientBalance(callback);
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

  auto callback = std::bind(&Contribution::StartAutoContribute,
      this,
      _1);

  monthly_->Process(callback);
}

void Contribution::StartAutoContribute(const ledger::Result result) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_INFO) << "Monthly contribution failed";
  }

  ac_->Process();
}

void Contribution::OnBalance(
    const std::string& contribution_queue,
    const ledger::Result result,
    ledger::BalancePtr info) {
  auto const queue =
      braveledger_bind_util::FromStringToContributionQueue(contribution_queue);
  if (result != ledger::Result::LEDGER_OK || !info) {
    queue_in_progress_ = false;
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
         "We couldn't get balance from the server.";
    return;
  }

  Process(queue->Clone(), std::move(info));
}


void Contribution::Start(ledger::ContributionQueuePtr info) {
  const auto info_converted =
      braveledger_bind_util::FromContributionQueueToString(std::move(info));
  ledger_->FetchBalance(
      std::bind(&Contribution::OnBalance,
                this,
                info_converted,
                _1,
                _2));
}

void Contribution::OnTimer(uint32_t timer_id) {
  unverified_->OnTimer(timer_id);
  uphold_->OnTimer(timer_id);

  for (std::pair<std::string, uint32_t> const& value : retry_timers_) {
    if (value.second != timer_id) {
      continue;
    }

    std::string contribution_id = value.first;
    retry_timers_[contribution_id] = 0u;

    auto callback = std::bind(&Contribution::SetRetryCounter,
        this,
        _1);
    ledger_->GetContributionInfo(contribution_id, callback);
    return;
  }

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

  if (contribution_id.empty()) {
    return;
  }

  auto save_callback = std::bind(&Contribution::ContributionCompletedSaved,
      this,
      _1);

  ledger_->UpdateContributionInfoStepAndCount(
      contribution_id,
      ConvertResultIntoContributionStep(result),
      -1,
      save_callback);
}

void Contribution::ContributionCompletedSaved(const ledger::Result result) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_INFO)
        << "Contribution step and count failed";
  }
}

void Contribution::ContributeUnverifiedPublishers() {
  unverified_->Contribute();
}

void Contribution::OneTimeTip(
    const std::string& publisher_key,
    const double amount,
    ledger::ResultCallback callback) {
  tip_->Process(publisher_key, amount, callback);
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

void Contribution::CreateNewEntry(
    const std::string& wallet_type,
    ledger::BalancePtr balance,
    ledger::ContributionQueuePtr queue) {
  if (!queue || queue->publishers.empty() || !balance || wallet_type.empty()) {
    DeleteContributionQueue(queue->id);
    return;
  }

  const double wallet_balance =
      braveledger_wallet::Balance::GetPerWalletBalance(
          wallet_type,
          balance->wallets);
  if (wallet_balance == 0) {
    CreateNewEntry(
        GetNextProcessor(wallet_type),
        std::move(balance),
        std::move(queue));
    return;
  }

  const std::string contribution_id = base::GenerateGUID();

  auto contribution = ledger::ContributionInfo::New();
  const uint64_t now = static_cast<uint64_t>(base::Time::Now().ToDoubleT());
  contribution->contribution_id = contribution_id;
  contribution->amount = queue->amount;
  contribution->type = queue->type;
  contribution->step = ledger::ContributionStep::STEP_START;
  contribution->retry_count = 0;
  contribution->created_at = now;
  contribution->processor = GetProcessor(wallet_type);

  ledger::ContributionQueuePublisherList queue_publishers;
  for (auto& item : queue->publishers) {
    queue_publishers.push_back(item->Clone());
  }

  ledger::ContributionQueuePublisherList publishers_new;
  ledger::ContributionQueuePublisherList publishers_left;
  if (wallet_balance < queue->amount) {
    contribution->amount = wallet_balance;
    queue->amount = queue->amount - wallet_balance;

    if (queue->type == ledger::RewardsType::RECURRING_TIP ||
        queue->type == ledger::RewardsType::ONE_TIME_TIP) {
      AdjustPublisherListAmounts(
          std::move(queue_publishers),
          &publishers_new,
          &publishers_left,
          wallet_balance);
      queue->publishers = std::move(publishers_left);
    } else {
      publishers_new = std::move(queue_publishers);
    }
  } else {
    publishers_new = std::move(queue_publishers);
    queue->amount = 0;
  }

  ledger::ContributionPublisherList publisher_list;
  for (auto& item : publishers_new) {
    auto publisher = ledger::ContributionPublisher::New();
    publisher->contribution_id = contribution_id;
    publisher->publisher_key = item->publisher_key;
    publisher->total_amount =
        (item->amount_percent * contribution->amount) / 100;
    publisher->contributed_amount = 0;
    publisher_list.push_back(std::move(publisher));
  }

  contribution->publishers = std::move(publisher_list);

  auto save_callback = std::bind(&Contribution::OnEntrySaved,
      this,
      _1,
      contribution->contribution_id,
      wallet_type,
      *balance,
      braveledger_bind_util::FromContributionQueueToString(queue->Clone()));

  ledger_->SaveContributionInfo(
      contribution->Clone(),
      save_callback);
}

void Contribution::OnEntrySaved(
    const ledger::Result result,
    const std::string& contribution_id,
    const std::string& wallet_type,
    const ledger::Balance& balance,
    const std::string& queue_string) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Contribution was not saved correctly";
    return;
  }

  auto queue = braveledger_bind_util::FromStringToContributionQueue(
      queue_string);

  if (!queue) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Queue was not converted successfully";
    return;
  }

  if (wallet_type == ledger::kWalletUnBlinded) {
    auto result_callback = std::bind(&Contribution::Result,
      this,
      _1,
      contribution_id);

    StartUnblinded(contribution_id, result_callback);
  } else if (wallet_type == ledger::kWalletAnonymous) {
    auto wallet = ledger::ExternalWallet::New();
    wallet->type = wallet_type;

    auto result_callback = std::bind(&Contribution::Result,
      this,
      _1,
      contribution_id);

    sku_->AnonUserFunds(contribution_id, std::move(wallet), result_callback);
  } else if (wallet_type == ledger::kWalletUphold) {
    auto result_callback = std::bind(&Contribution::Result,
        this,
        _1,
        contribution_id);

    external_wallet_->Process(contribution_id, result_callback);
  }

  if (queue->amount > 0) {
    auto save_callback = std::bind(&Contribution::OnQueueSaved,
      this,
      _1,
      wallet_type,
      balance,
      braveledger_bind_util::FromContributionQueueToString(queue->Clone()));

    ledger_->SaveContributionQueue(queue->Clone(), save_callback);
  } else {
    DeleteContributionQueue(queue->id);
  }
}

void Contribution::OnQueueSaved(
    const ledger::Result result,
    const std::string& wallet_type,
    const ledger::Balance& balance,
    const std::string& queue_string) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Queue was not saved successfully";
    return;
  }

  auto queue = braveledger_bind_util::FromStringToContributionQueue(
      queue_string);

  if (!queue) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Queue was not converted successfully";
    return;
  }

  CreateNewEntry(
      GetNextProcessor(wallet_type),
      ledger::Balance::New(balance),
      std::move(queue));
}

void Contribution::Process(
    ledger::ContributionQueuePtr queue,
    ledger::BalancePtr balance) {
  if (!queue) {
    return;
  }

  if (queue->amount == 0 || queue->publishers.empty()) {
    DeleteContributionQueue(queue->id);
    return;
  }

  const auto have_enough_balance = HaveEnoughFundsToContribute(
      &queue->amount,
      queue->partial,
      balance->total);

  if (!have_enough_balance) {
    DeleteContributionQueue(queue->id);
    return;
  }

  if (queue->amount == 0 || queue->publishers.empty()) {
    DeleteContributionQueue(queue->id);
    return;
  }

  CreateNewEntry(
      GetNextProcessor(""),
      balance->Clone(),
      queue->Clone());
}

void Contribution::TransferFunds(
    const ledger::SKUTransaction& transaction,
    const std::string& destination,
    ledger::ExternalWalletPtr wallet,
    ledger::TransactionCallback callback) {
  if (!wallet) {
     BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Wallet is null";
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  if (wallet->type == ledger::kWalletUphold) {
    uphold_->TransferFunds(
        transaction.amount,
        destination,
        std::move(wallet),
        callback);
    return;
  }

  if (wallet->type == ledger::kWalletAnonymous) {
    anon_card_->SendTransaction(
        transaction.amount,
        transaction.order_id,
        destination,
        callback);
    return;
  }

  if (wallet->type == ledger::kWalletUnBlinded) {
    sku_->Merchant(transaction, callback);
    return;
  }

  NOTREACHED();
  BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
      << "Wallet type not supported: " << wallet->type;
}

void Contribution::SKUAutoContribution(
    const std::string& contribution_id,
    ledger::ExternalWalletPtr wallet,
    ledger::ResultCallback callback) {
  sku_->AutoContribution(contribution_id, std::move(wallet), callback);
}

void Contribution::StartUnblinded(
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  unblinded_->Start(contribution_id, callback);
}

void Contribution::RetryUnblinded(
    const std::string& contribution_id,
    ledger::ResultCallback callback) {

  auto get_callback = std::bind(&Contribution::RetryUnblindedContribution,
      this,
      _1,
      callback);

  ledger_->GetContributionInfo(contribution_id, get_callback);
}

void Contribution::RetryUnblindedContribution(
    ledger::ContributionInfoPtr contribution,
    ledger::ResultCallback callback) {
  unblinded_->Retry(std::move(contribution), callback);
}

void Contribution::Result(
    const ledger::Result result,
    const std::string& contribution_id) {
  if (result == ledger::Result::RETRY_SHORT) {
    SetRetryTimer(contribution_id, 5);
    return;
  }

  if (result == ledger::Result::RETRY) {
    SetRetryTimer(contribution_id);
    return;
  }

  auto get_callback = std::bind(&Contribution::FinishContribution,
      this,
      _1,
      result);

  ledger_->GetContributionInfo(contribution_id, get_callback);
}

void Contribution::FinishContribution(
    ledger::ContributionInfoPtr contribution,
    const ledger::Result result) {
  if (!contribution) {
    return;
  }

  ledger_->ContributionCompleted(
      result,
      contribution->amount,
      contribution->contribution_id,
      contribution->type);
}

void Contribution::SetRetryTimer(
    const std::string& contribution_id,
    const uint64_t& start_timer_in) {
  if (contribution_id.empty()) {
    return;
  }

  if (!retry_timers_[contribution_id]) {
    retry_timers_[contribution_id] = 0u;
  }

  uint64_t timer_seconds = start_timer_in;
  if (ledger::short_retries) {
    timer_seconds = 1;
  } else if (start_timer_in == 0) {
    timer_seconds = brave_base::random::Geometric(45);
  }

  BLOG(ledger_, ledger::LogLevel::LOG_INFO)
    << "Timer for contribution retry ("
    << contribution_id
    << ") will start in "
    << timer_seconds;

  ledger_->SetTimer(timer_seconds, &retry_timers_[contribution_id]);
}

void Contribution::SetRetryCounter(ledger::ContributionInfoPtr contribution) {
  if (contribution->retry_count == 3) {
    ledger_->ContributionCompleted(
        ledger::Result::LEDGER_ERROR,
        contribution->amount,
        contribution->contribution_id,
        contribution->type);
    return;
  }

  auto save_callback = std::bind(&Contribution::Retry,
      this,
      _1,
      braveledger_bind_util::FromContributionToString(contribution->Clone()));

  ledger_->UpdateContributionInfoStepAndCount(
      contribution->contribution_id,
      contribution->step,
      contribution->retry_count + 1,
      save_callback);
}

void Contribution::Retry(
    const ledger::Result result,
    const std::string& contribution_string) {
  if (result != ledger::Result::LEDGER_OK) {
    return;
  }

  auto contribution = braveledger_bind_util::FromStringToContribution(
      contribution_string);

  if (!contribution) {
    return;
  }

  // negative steps are final steps, nothing to retry
  if (static_cast<int>(contribution->step) < 0) {
    return;
  }

  BLOG(ledger_, ledger::LogLevel::LOG_INFO)
      << "Retrying contribution ("
      << contribution->contribution_id
      << ") on step "
      << contribution->step;

  auto result_callback = std::bind(&Contribution::Result,
    this,
    _1,
    contribution->contribution_id);

  switch (contribution->processor) {
    case ledger::ContributionProcessor::BRAVE_TOKENS: {
      unblinded_->Retry(contribution->Clone(), result_callback);
      return;
    }
    case ledger::ContributionProcessor::UPHOLD: {
      if (contribution->type == ledger::RewardsType::AUTO_CONTRIBUTE) {
        sku_->Retry(contribution->Clone(), result_callback);
        return;
      }

      external_wallet_->Retry(contribution->Clone(), result_callback);
      return;
    }
    case ledger::ContributionProcessor::BRAVE_USER_FUNDS: {
      sku_->Retry(contribution->Clone(), result_callback);
      return;
    }
    case ledger::ContributionProcessor::NONE: {
      Result(ledger::Result::LEDGER_ERROR, contribution->contribution_id);
      return;
    }
  }
}

}  // namespace braveledger_contribution
