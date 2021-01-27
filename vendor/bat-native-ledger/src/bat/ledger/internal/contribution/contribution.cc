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
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/contribution/contribution.h"
#include "bat/ledger/internal/contribution/contribution_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/publisher/publisher_status_helper.h"
#include "bat/ledger/internal/wallet/wallet_balance.h"
#include "bat/ledger/option_keys.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {
ledger::type::ContributionStep ConvertResultIntoContributionStep(
    const ledger::type::Result result) {
  switch (result) {
    case ledger::type::Result::LEDGER_OK: {
      return ledger::type::ContributionStep::STEP_COMPLETED;
    }
    case ledger::type::Result::AC_TABLE_EMPTY: {
      return ledger::type::ContributionStep::STEP_AC_TABLE_EMPTY;
    }
    case ledger::type::Result::NOT_ENOUGH_FUNDS: {
      return ledger::type::ContributionStep::STEP_NOT_ENOUGH_FUNDS;
    }
    case ledger::type::Result::REWARDS_OFF: {
      return ledger::type::ContributionStep::STEP_REWARDS_OFF;
    }
    case ledger::type::Result::AC_OFF: {
      return ledger::type::ContributionStep::STEP_AC_OFF;
    }
    case ledger::type::Result::TOO_MANY_RESULTS: {
      return ledger::type::ContributionStep::STEP_RETRY_COUNT;
    }
    default: {
      return ledger::type::ContributionStep::STEP_FAILED;
    }
  }
}

}  // namespace

namespace ledger {
namespace contribution {

Contribution::Contribution(LedgerImpl* ledger) :
    ledger_(ledger),
    unverified_(std::make_unique<Unverified>(ledger)),
    unblinded_(std::make_unique<Unblinded>(ledger)),
    sku_(std::make_unique<ContributionSKU>(ledger)),
    monthly_(std::make_unique<ContributionMonthly>(ledger)),
    ac_(std::make_unique<ContributionAC>(ledger)),
    tip_(std::make_unique<ContributionTip>(ledger)),
    anon_card_(std::make_unique<ContributionAnonCard>(ledger)) {
  DCHECK(ledger_);
  external_wallet_ = std::make_unique<ContributionExternalWallet>(ledger);
}

Contribution::~Contribution() = default;

void Contribution::Initialize() {
  ledger_->uphold()->Initialize();
  ledger_->bitflyer()->Initialize();

  CheckContributionQueue();
  CheckNotCompletedContributions();
}

void Contribution::CheckContributionQueue() {
  base::TimeDelta delay = ledger::is_testing
      ? base::TimeDelta::FromSeconds(1)
      : util::GetRandomizedDelay(
          base::TimeDelta::FromSeconds(15));

  BLOG(1, "Queue timer set for " << delay);

  queue_timer_.Start(FROM_HERE, delay,
      base::BindOnce(&Contribution::ProcessContributionQueue,
          base::Unretained(this)));
}

void Contribution::ProcessContributionQueue() {
  if (queue_in_progress_) {
    return;
  }

  const auto callback = std::bind(&Contribution::OnProcessContributionQueue,
      this,
      _1);
  ledger_->database()->GetFirstContributionQueue(callback);
}

void Contribution::OnProcessContributionQueue(
    type::ContributionQueuePtr info) {
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

  ledger_->database()->GetNotCompletedContributions(get_callback);
}

void Contribution::NotCompletedContributions(
    type::ContributionInfoList list) {
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
  ledger_->state()->ResetReconcileStamp();
  SetReconcileTimer();
}

void Contribution::StartMonthlyContribution() {
  if (ledger_->ledger_client()->GetBooleanOption(
          option::kContributionsDisabledForBAPMigration)) {
    BLOG(1, "Monthly contributions disabled for BAP migration");
    return;
  }

  const auto reconcile_stamp = ledger_->state()->GetReconcileStamp();
  ResetReconcileStamp();

  BLOG(1, "Starting monthly contribution");

  auto callback = std::bind(&Contribution::StartAutoContribute,
      this,
      _1,
      reconcile_stamp);

  monthly_->Process(callback);
}

void Contribution::StartAutoContribute(
    const type::Result result,
    const uint64_t reconcile_stamp) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Monthly contribution failed");
  }

  ac_->Process(reconcile_stamp);
}

void Contribution::OnBalance(
    const type::Result result,
    type::BalancePtr info,
    std::shared_ptr<type::ContributionQueuePtr> shared_queue) {
  if (result != type::Result::LEDGER_OK || !shared_queue) {
    queue_in_progress_ = false;
    BLOG(0, "We couldn't get balance from the server.");
    return;
  }

  Process(std::move(*shared_queue), std::move(info));
}

void Contribution::Start(type::ContributionQueuePtr info) {
  auto fetch_callback = std::bind(&Contribution::OnBalance,
      this,
      _1,
      _2,
      std::make_shared<type::ContributionQueuePtr>(std::move(info)));
  ledger_->wallet()->FetchBalance(fetch_callback);
}

void Contribution::SetReconcileTimer() {
  if (last_reconcile_timer_.IsRunning()) {
    return;
  }

  uint64_t now = std::time(nullptr);
  uint64_t next_reconcile_stamp = ledger_->state()->GetReconcileStamp();

  base::TimeDelta delay;
  if (next_reconcile_stamp > now) {
    delay = base::TimeDelta::FromSeconds(next_reconcile_stamp - now);
  }

  BLOG(1, "Last reconcile timer set for " << delay);

  last_reconcile_timer_.Start(FROM_HERE, delay,
      base::BindOnce(&Contribution::StartMonthlyContribution,
          base::Unretained(this)));
}

void Contribution::ContributionCompleted(
    const type::Result result,
    type::ContributionInfoPtr contribution) {
  if (!contribution) {
    BLOG(0, "Contribution is null");
    return;
  }

  // TODO(https://github.com/brave/brave-browser/issues/7717)
  // rename to ContributionCompleted
  ledger_->ledger_client()->OnReconcileComplete(
      result,
      contribution->Clone());

  if (result == type::Result::LEDGER_OK) {
    ledger_->database()->SaveBalanceReportInfoItem(
        util::GetCurrentMonth(),
        util::GetCurrentYear(),
        GetReportTypeFromRewardsType(contribution->type),
        contribution->amount,
        [](const type::Result){});
  }

  auto save_callback = std::bind(&Contribution::ContributionCompletedSaved,
      this,
      _1,
      contribution->contribution_id);

  ledger_->database()->UpdateContributionInfoStepAndCount(
      contribution->contribution_id,
      ConvertResultIntoContributionStep(result),
      -1,
      save_callback);
}

void Contribution::ContributionCompletedSaved(
    const type::Result result,
    const std::string& contribution_id) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Contribution step and count failed");
  }

  auto callback = std::bind(&Contribution::OnMarkUnblindedTokensAsSpendable,
      this,
      _1,
      contribution_id);
  ledger_->database()->MarkUnblindedTokensAsSpendable(
      contribution_id,
      callback);
}

void Contribution::ContributeUnverifiedPublishers() {
  unverified_->Contribute();
}

void Contribution::OneTimeTip(
    const std::string& publisher_key,
    const double amount,
    ledger::ResultCallback callback) {
  if (ledger_->ledger_client()->GetBooleanOption(
          option::kContributionsDisabledForBAPMigration)) {
    BLOG(1, "One-time tips disabled for BAP migration");
    callback(type::Result::LEDGER_ERROR);
    return;
  }
  tip_->Process(publisher_key, amount, callback);
}

void Contribution::OnMarkContributionQueueAsComplete(
    const type::Result result) {
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

  ledger_->database()->MarkContributionQueueAsComplete(id, callback);
}

void Contribution::CreateNewEntry(
    const std::string& wallet_type,
    type::BalancePtr balance,
    type::ContributionQueuePtr queue) {
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
      wallet::WalletBalance::GetPerWalletBalance(
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

  if (wallet_type == constant::kWalletBitflyer &&
      queue->type == type::RewardsType::AUTO_CONTRIBUTE) {
    BLOG(1, "AC is not supported for bitFlyer wallets");
    CreateNewEntry(GetNextProcessor(wallet_type), std::move(balance),
                   std::move(queue));
    return;
  }

  const std::string contribution_id = base::GenerateGUID();

  auto contribution = type::ContributionInfo::New();
  const uint64_t now = util::GetCurrentTimeStamp();
  contribution->contribution_id = contribution_id;
  contribution->amount = queue->amount;
  contribution->type = queue->type;
  contribution->step = type::ContributionStep::STEP_START;
  contribution->retry_count = 0;
  contribution->created_at = now;
  contribution->processor = GetProcessor(wallet_type);

  type::ContributionQueuePublisherList queue_publishers;
  for (auto& item : queue->publishers) {
    queue_publishers.push_back(item->Clone());
  }

  if (wallet_balance < queue->amount) {
    contribution->amount = wallet_balance;
    queue->amount = queue->amount - wallet_balance;
  } else {
    queue->amount = 0;
  }

  BLOG(1, "Creating contribution for wallet type "
              << wallet_type << " (amount: " << contribution->amount
              << ", type: " << queue->type << ")");

  type::ContributionPublisherList publisher_list;
  for (const auto& item : queue_publishers) {
    auto publisher = type::ContributionPublisher::New();
    publisher->contribution_id = contribution_id;
    publisher->publisher_key = item->publisher_key;
    publisher->total_amount =
        (item->amount_percent * contribution->amount) / 100;
    publisher->contributed_amount = 0;
    publisher_list.push_back(std::move(publisher));
  }

  contribution->publishers = std::move(publisher_list);

  auto shared_queue =
      std::make_shared<type::ContributionQueuePtr>(std::move(queue));

  auto save_callback = std::bind(&Contribution::OnEntrySaved,
      this,
      _1,
      contribution->contribution_id,
      wallet_type,
      *balance,
      shared_queue);

  ledger_->database()->SaveContributionInfo(
      contribution->Clone(),
      save_callback);
}

void Contribution::OnEntrySaved(
    const type::Result result,
    const std::string& contribution_id,
    const std::string& wallet_type,
    const type::Balance& balance,
    std::shared_ptr<type::ContributionQueuePtr> shared_queue) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Contribution was not saved correctly");
    return;
  }

  if (!shared_queue) {
    BLOG(0, "Queue is null");
    return;
  }

  if (wallet_type == constant::kWalletUnBlinded) {
    auto result_callback = std::bind(&Contribution::Result,
      this,
      _1,
      contribution_id);

    StartUnblinded(
        {type::CredsBatchType::PROMOTION},
        contribution_id,
        result_callback);
  } else if (wallet_type == constant::kWalletAnonymous) {
    auto result_callback = std::bind(&Contribution::Result,
      this,
      _1,
      contribution_id);

    sku_->AnonUserFunds(contribution_id, wallet_type, result_callback);
  } else if (wallet_type == constant::kWalletUphold ||
             wallet_type == constant::kWalletBitflyer) {
    auto result_callback = std::bind(&Contribution::Result,
        this,
        _1,
        contribution_id);

    external_wallet_->Process(contribution_id, result_callback);
  }

  if ((*shared_queue)->amount > 0) {
    auto save_callback = std::bind(&Contribution::OnQueueSaved,
      this,
      _1,
      wallet_type,
      balance,
      shared_queue);

    ledger_->database()->SaveContributionQueue(
        (*shared_queue)->Clone(),
        save_callback);
  } else {
    MarkContributionQueueAsComplete((*shared_queue)->id);
  }
}

void Contribution::OnQueueSaved(
    const type::Result result,
    const std::string& wallet_type,
    const type::Balance& balance,
    std::shared_ptr<type::ContributionQueuePtr> shared_queue) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Queue was not saved successfully");
    return;
  }

  if (!shared_queue) {
    BLOG(0, "Queue was not converted successfully");
    return;
  }

  CreateNewEntry(
      GetNextProcessor(wallet_type),
      type::Balance::New(balance),
      std::move(*shared_queue));
}

void Contribution::Process(
    type::ContributionQueuePtr queue,
    type::BalancePtr balance) {
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
    const type::SKUTransaction& transaction,
    const std::string& destination,
    const std::string& wallet_type,
    client::TransactionCallback callback) {
  if (wallet_type == constant::kWalletUphold) {
    ledger_->uphold()->TransferFunds(
        transaction.amount,
        destination,
        callback);
    return;
  }

  if (wallet_type == constant::kWalletBitflyer) {
    ledger_->bitflyer()->TransferFunds(transaction.amount, destination,
                                       callback);
    return;
  }

  if (wallet_type == constant::kWalletAnonymous) {
    anon_card_->SendTransaction(
        transaction.amount,
        transaction.order_id,
        destination,
        callback);
    return;
  }

  if (wallet_type == constant::kWalletUnBlinded) {
    sku_->Merchant(transaction, callback);
    return;
  }

  NOTREACHED();
  BLOG(0, "Wallet type not supported: " << wallet_type);
}

void Contribution::SKUAutoContribution(
    const std::string& contribution_id,
    const std::string& wallet_type,
    ledger::ResultCallback callback) {
  sku_->AutoContribution(contribution_id, wallet_type, callback);
}

void Contribution::StartUnblinded(
    const std::vector<type::CredsBatchType>& types,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  unblinded_->Start(types, contribution_id, callback);
}

void Contribution::RetryUnblinded(
    const std::vector<type::CredsBatchType>& types,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {

  auto get_callback = std::bind(&Contribution::RetryUnblindedContribution,
      this,
      _1,
      types,
      callback);

  ledger_->database()->GetContributionInfo(contribution_id, get_callback);
}

void Contribution::RetryUnblindedContribution(
    type::ContributionInfoPtr contribution,
    const std::vector<type::CredsBatchType>& types,
    ledger::ResultCallback callback) {
  unblinded_->Retry(types, std::move(contribution), callback);
}

void Contribution::Result(
    const type::Result result,
    const std::string& contribution_id) {
  if (result == type::Result::RETRY_SHORT) {
    SetRetryTimer(contribution_id, base::TimeDelta::FromSeconds(5));
    return;
  }

  if (result == type::Result::RETRY) {
    SetRetryTimer(
        contribution_id,
        util::GetRandomizedDelay(
            base::TimeDelta::FromSeconds(45)));
    return;
  }

  auto get_callback = std::bind(&Contribution::OnResult,
      this,
      _1,
      result);

  ledger_->database()->GetContributionInfo(contribution_id, get_callback);
}

void Contribution::OnResult(
    type::ContributionInfoPtr contribution,
    const type::Result result) {
  if (!contribution) {
    BLOG(0, "Contribution is null");
    return;
  }

  if (result == type::Result::RETRY_LONG) {
    if (contribution->processor ==
        type::ContributionProcessor::BRAVE_TOKENS) {
      SetRetryTimer(
          contribution->contribution_id,
          util::GetRandomizedDelay(
              base::TimeDelta::FromSeconds(45)));
    } else {
      SetRetryTimer(
          contribution->contribution_id,
          util::GetRandomizedDelay(
              base::TimeDelta::FromSeconds(450)));
    }

    return;
  }

  ledger_->contribution()->ContributionCompleted(
      result,
      std::move(contribution));
}

void Contribution::SetRetryTimer(
    const std::string& contribution_id,
    base::TimeDelta delay) {
  if (contribution_id.empty()) {
    BLOG(0, "Contribution id is empty");
    return;
  }

  if (ledger::short_retries) {
    delay = base::TimeDelta::FromSeconds(1);
  }

  BLOG(1, "Timer for contribution retry (" << contribution_id << ") "
      "set for " << delay);

  retry_timers_[contribution_id].Start(FROM_HERE, delay,
      base::BindOnce(&Contribution::OnRetryTimerElapsed,
          base::Unretained(this),
          contribution_id));
}

void Contribution::OnRetryTimerElapsed(const std::string& contribution_id) {
  retry_timers_.erase(contribution_id);

  auto callback = std::bind(&Contribution::SetRetryCounter,
      this,
      _1);

  ledger_->database()->GetContributionInfo(contribution_id, callback);
}

void Contribution::SetRetryCounter(type::ContributionInfoPtr contribution) {
  if (!contribution) {
    BLOG(0, "Contribution is null");
    return;
  }

  if (contribution->retry_count == 3 &&
      contribution->step != type::ContributionStep::STEP_PREPARE) {
    BLOG(0, "Contribution failed after 3 retries");
    ledger_->contribution()->ContributionCompleted(
        type::Result::TOO_MANY_RESULTS,
        std::move(contribution));
    return;
  }

  auto save_callback = std::bind(&Contribution::Retry,
      this,
      _1,
      std::make_shared<type::ContributionInfoPtr>(contribution->Clone()));

  ledger_->database()->UpdateContributionInfoStepAndCount(
      contribution->contribution_id,
      contribution->step,
      contribution->retry_count + 1,
      save_callback);
}

void Contribution::OnMarkUnblindedTokensAsSpendable(
    const type::Result result,
    const std::string& contribution_id) {
  BLOG_IF(
      1,
      result != type::Result::LEDGER_OK,
      "Failed to mark unblinded tokens as unreserved for contribution "
          << contribution_id);
}

void Contribution::Retry(
    const type::Result result,
    std::shared_ptr<type::ContributionInfoPtr> shared_contribution) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Retry count update failed");
    return;
  }

  if (!shared_contribution) {
    BLOG(0, "Contribution is null");
    return;
  }

  // negative steps are final steps, nothing to retry
  if (static_cast<int>((*shared_contribution)->step) < 0) {
    return;
  }

  if ((*shared_contribution)->type == type::RewardsType::AUTO_CONTRIBUTE &&
      !ledger_->state()->GetAutoContributeEnabled()) {
    BLOG(1, "AC is disabled, completing contribution");
    ledger_->contribution()->ContributionCompleted(
        type::Result::AC_OFF,
        std::move(*shared_contribution));
    return;
  }

  BLOG(1, "Retrying contribution (" << (*shared_contribution)->contribution_id
      << ") on step " << (*shared_contribution)->step);

  auto result_callback = std::bind(&Contribution::Result,
    this,
    _1,
    (*shared_contribution)->contribution_id);

  switch ((*shared_contribution)->processor) {
    case type::ContributionProcessor::BRAVE_TOKENS: {
      RetryUnblindedContribution(
          (*shared_contribution)->Clone(),
          {type::CredsBatchType::PROMOTION},
          result_callback);
      return;
    }
    case type::ContributionProcessor::UPHOLD:
    case type::ContributionProcessor::BITFLYER: {
      if ((*shared_contribution)->type ==
          type::RewardsType::AUTO_CONTRIBUTE) {
        sku_->Retry((*shared_contribution)->Clone(), result_callback);
        return;
      }

      external_wallet_->Retry((*shared_contribution)->Clone(), result_callback);
      return;
    }
    case type::ContributionProcessor::BRAVE_USER_FUNDS: {
      sku_->Retry((*shared_contribution)->Clone(), result_callback);
      return;
    }
    case type::ContributionProcessor::NONE: {
      Result(
          type::Result::LEDGER_ERROR,
          (*shared_contribution)->contribution_id);
      return;
    }
  }
}

void Contribution::GetRecurringTips(
    ledger::PublisherInfoListCallback callback) {
  ledger_->database()->GetRecurringTips([this, callback](
      type::PublisherInfoList list) {
    // The publisher status field may be expired. Attempt to refresh
    // expired publisher status values before executing callback.
    publisher::RefreshPublisherStatus(
        ledger_,
        std::move(list),
        callback);
  });
}

}  // namespace contribution
}  // namespace ledger
