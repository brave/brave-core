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
  if (list.empty()) {
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
  const auto reconcile_stamp = ledger_->GetReconcileStamp();
  ResetReconcileStamp();

  if (!ledger_->GetRewardsMainEnabled()) {
    return;
  }

  BLOG(1, "Staring monthly contribution");

  auto callback = std::bind(&Contribution::StartAutoContribute,
      this,
      _1,
      reconcile_stamp);

  monthly_->Process(callback);
}

void Contribution::StartAutoContribute(
    const ledger::Result result,
    const uint64_t reconcile_stamp) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Monthly contribution failed");
  }

  ac_->Process(reconcile_stamp);
}

void Contribution::OnBalance(
    const std::string& contribution_queue,
    const ledger::Result result,
    ledger::BalancePtr info) {
  auto const queue =
      braveledger_bind_util::FromStringToContributionQueue(contribution_queue);
  if (result != ledger::Result::LEDGER_OK || !info) {
    queue_in_progress_ = false;
    BLOG(0, "We couldn't get balance from the server.");
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

  for (const auto& value : retry_timers_) {
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

  BLOG(1, "Timer will start in " << start_timer_in);

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
    BLOG(0, "Contribution id is empty");
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
    BLOG(0, "Contribution step and count failed");
    return;
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

void Contribution::OnMarkContributionQueueAsComplete(
    const ledger::Result result) {
  queue_in_progress_ = false;
  CheckContributionQueue();
}

void Contribution::MarkContributionQueueAsComplete(const std::string& id) {
  if (id.empty()) {
    BLOG(0, "Queue id is empty");
    return;
  }

  auto callback = std::bind(&Contribution::OnMarkContributionQueueAsComplete,
      this,
      _1);

  ledger_->MarkContributionQueueAsComplete(id, callback);
}

void Contribution::CreateNewEntry(
    const std::string& wallet_type,
    ledger::BalancePtr balance,
    ledger::ContributionQueuePtr queue) {
  if (!queue) {
    BLOG(1, "Queue is null");
    return;
  }

  if (queue->publishers.empty() || !balance || wallet_type.empty()) {
    BLOG(0, "Queue data is wrong");
    MarkContributionQueueAsComplete(queue->id);
    return;
  }

  const double wallet_balance =
      braveledger_wallet::Balance::GetPerWalletBalance(
          wallet_type,
          balance->wallets);
  if (wallet_balance == 0) {
    BLOG(1, "Wallet balance is 0 for " << wallet_type);
    CreateNewEntry(
        GetNextProcessor(wallet_type),
        std::move(balance),
        std::move(queue));
    return;
  }


  BLOG(1, "Creating contribution(" << wallet_type << ") for " <<
      queue->amount << " type " << queue->type);

  const std::string contribution_id = base::GenerateGUID();

  auto contribution = ledger::ContributionInfo::New();
  const uint64_t now = braveledger_time_util::GetCurrentTimeStamp();
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

  if (wallet_balance < queue->amount) {
    contribution->amount = wallet_balance;
    queue->amount = queue->amount - wallet_balance;
  } else {
    queue->amount = 0;
  }

  ledger::ContributionPublisherList publisher_list;
  for (const auto& item : queue_publishers) {
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
    BLOG(0, "Contribution was not saved correctly");
    return;
  }

  auto queue = braveledger_bind_util::FromStringToContributionQueue(
      queue_string);

  if (!queue) {
    BLOG(0, "Queue was not converted successfully");
    return;
  }

  if (wallet_type == ledger::kWalletUnBlinded) {
    auto result_callback = std::bind(&Contribution::Result,
      this,
      _1,
      contribution_id);

    StartUnblinded(
        {ledger::CredsBatchType::PROMOTION},
        contribution_id,
        result_callback);
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
    MarkContributionQueueAsComplete(queue->id);
  }
}

void Contribution::OnQueueSaved(
    const ledger::Result result,
    const std::string& wallet_type,
    const ledger::Balance& balance,
    const std::string& queue_string) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Queue was not saved successfully");
    return;
  }

  auto queue = braveledger_bind_util::FromStringToContributionQueue(
      queue_string);

  if (!queue) {
    BLOG(0, "Queue was not converted successfully");
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
    BLOG(0, "Queue is null");
    return;
  }

  if (!balance) {
    BLOG(0, "Balance is null");
    return;
  }

  if (queue->amount == 0 || queue->publishers.empty()) {
    BLOG(0, "Amount/publisher is empty");
    MarkContributionQueueAsComplete(queue->id);
    return;
  }

  const auto have_enough_balance = HaveEnoughFundsToContribute(
      &queue->amount,
      queue->partial,
      balance->total);

  if (!have_enough_balance) {
    BLOG(1, "Not enough balance");
    MarkContributionQueueAsComplete(queue->id);
    return;
  }

  if (queue->amount == 0) {
    BLOG(0, "Amount is 0");
    MarkContributionQueueAsComplete(queue->id);
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
     BLOG(0, "Wallet is null");
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
  BLOG(0, "Wallet type not supported: " << wallet->type);
}

void Contribution::SKUAutoContribution(
    const std::string& contribution_id,
    ledger::ExternalWalletPtr wallet,
    ledger::ResultCallback callback) {
  sku_->AutoContribution(contribution_id, std::move(wallet), callback);
}

void Contribution::StartUnblinded(
    const std::vector<ledger::CredsBatchType>& types,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  unblinded_->Start(types, contribution_id, callback);
}

void Contribution::RetryUnblinded(
    const std::vector<ledger::CredsBatchType>& types,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {

  auto get_callback = std::bind(&Contribution::RetryUnblindedContribution,
      this,
      _1,
      types,
      callback);

  ledger_->GetContributionInfo(contribution_id, get_callback);
}

void Contribution::RetryUnblindedContribution(
    ledger::ContributionInfoPtr contribution,
    const std::vector<ledger::CredsBatchType>& types,
    ledger::ResultCallback callback) {
  unblinded_->Retry(types, std::move(contribution), callback);
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

  auto get_callback = std::bind(&Contribution::OnResult,
      this,
      _1,
      result);

  ledger_->GetContributionInfo(contribution_id, get_callback);
}

void Contribution::OnResult(
    ledger::ContributionInfoPtr contribution,
    const ledger::Result result) {
  if (!contribution) {
    BLOG(0, "Contribution is null");
    return;
  }

  if (result == ledger::Result::RETRY_LONG) {
    if (ledger::short_retries) {
      SetRetryTimer(contribution->contribution_id, 1);
      return;
    }

    if (contribution->processor ==
        ledger::ContributionProcessor::BRAVE_TOKENS) {
      SetRetryTimer(contribution->contribution_id);
    } else {
      SetRetryTimer(
          contribution->contribution_id,
          brave_base::random::Geometric(450));
    }

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
    const uint64_t start_timer_in) {
  if (contribution_id.empty()) {
    BLOG(0, "Contribution id is empty");
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

  BLOG(1, "Timer for contribution retry (" << contribution_id << ") will "
      "start in " << timer_seconds);

  ledger_->SetTimer(timer_seconds, &retry_timers_[contribution_id]);
}

void Contribution::SetRetryCounter(ledger::ContributionInfoPtr contribution) {
  if (!contribution) {
    BLOG(0, "Contribution is null");
    return;
  }

  if (contribution->retry_count == 3) {
    BLOG(0, "Contribution failed after 3 retries");
    auto callback = std::bind(&Contribution::OnMarkUnblindedTokensAsSpendable,
        this,
        _1,
        braveledger_bind_util::FromContributionToString(contribution->Clone()));
    ledger_->MarkUnblindedTokensAsSpendable(
        contribution->contribution_id,
        callback);
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

void Contribution::OnMarkUnblindedTokensAsSpendable(
    const ledger::Result result,
    const std::string& contribution_string) {
  auto contribution = braveledger_bind_util::FromStringToContribution(
      contribution_string);
  if (!contribution) {
    BLOG(0, "Contribution was not converted successfully");
    return;
  }

  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Failed to mark unblinded tokens as unreserved for contribution "
        << contribution->contribution_id);
  }

  // Even if we can't mark the tokens as unreserved, mark the
  // contribution as completed
  ledger_->ContributionCompleted(
      ledger::Result::LEDGER_ERROR,
      contribution->amount,
      contribution->contribution_id,
      contribution->type);
}

void Contribution::Retry(
    const ledger::Result result,
    const std::string& contribution_string) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Retry count update failed");
    return;
  }

  auto contribution = braveledger_bind_util::FromStringToContribution(
      contribution_string);

  if (!contribution) {
    BLOG(0, "Contribution is null");
    return;
  }

  // negative steps are final steps, nothing to retry
  if (static_cast<int>(contribution->step) < 0) {
    return;
  }

  BLOG(1, "Retrying contribution (" << contribution->contribution_id
      << ") on step " << contribution->step);

  auto result_callback = std::bind(&Contribution::Result,
    this,
    _1,
    contribution->contribution_id);

  switch (contribution->processor) {
    case ledger::ContributionProcessor::BRAVE_TOKENS: {
      RetryUnblindedContribution(
          contribution->Clone(),
          {ledger::CredsBatchType::PROMOTION},
          result_callback);
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
