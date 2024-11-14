/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/contribution/contribution.h"

#include <stdint.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/contribution/contribution_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/gemini/gemini.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/publisher/publisher_status_helper.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "brave/components/brave_rewards/core/uphold/uphold.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "brave/components/brave_rewards/core/wallet/wallet_balance.h"

namespace brave_rewards::internal {

namespace {

mojom::ContributionStep ConvertResultIntoContributionStep(
    mojom::Result result) {
  switch (result) {
    case mojom::Result::OK: {
      return mojom::ContributionStep::STEP_COMPLETED;
    }
    case mojom::Result::AC_TABLE_EMPTY: {
      return mojom::ContributionStep::STEP_AC_TABLE_EMPTY;
    }
    case mojom::Result::NOT_ENOUGH_FUNDS: {
      return mojom::ContributionStep::STEP_NOT_ENOUGH_FUNDS;
    }
    case mojom::Result::REWARDS_OFF: {
      return mojom::ContributionStep::STEP_REWARDS_OFF;
    }
    case mojom::Result::AC_OFF: {
      return mojom::ContributionStep::STEP_AC_OFF;
    }
    case mojom::Result::TOO_MANY_RESULTS: {
      return mojom::ContributionStep::STEP_RETRY_COUNT;
    }
    default: {
      return mojom::ContributionStep::STEP_FAILED;
    }
  }
}

}  // namespace

namespace contribution {

Contribution::ContributionRequest::ContributionRequest(
    std::string publisher_id,
    double amount,
    bool set_monthly,
    base::OnceCallback<void(bool)> callback)
    : publisher_id(std::move(publisher_id)),
      amount(amount),
      set_monthly(set_monthly),
      callback(std::move(callback)) {}

Contribution::ContributionRequest::ContributionRequest(
    ContributionRequest&& other) = default;

Contribution::ContributionRequest& Contribution::ContributionRequest::operator=(
    ContributionRequest&& other) = default;

Contribution::ContributionRequest::~ContributionRequest() = default;

Contribution::Contribution(RewardsEngine& engine)
    : engine_(engine),
      monthly_(engine),
      tip_(engine),
      external_wallet_(engine) {}

Contribution::~Contribution() = default;

void Contribution::Initialize() {
  engine_->uphold()->Initialize();
  engine_->bitflyer()->Initialize();
  engine_->gemini()->Initialize();

  CheckContributionQueue();
  CheckNotCompletedContributions();
}

void Contribution::CheckContributionQueue() {
  base::TimeDelta delay = engine_->options().is_testing
                              ? base::Seconds(1)
                              : util::GetRandomizedDelay(base::Seconds(15));

  engine_->Log(FROM_HERE) << "Queue timer set for " << delay;

  queue_timer_.Start(FROM_HERE, delay,
                     base::BindOnce(&Contribution::ProcessContributionQueue,
                                    weak_factory_.GetWeakPtr()));
}

void Contribution::ProcessContributionQueue() {
  if (queue_in_progress_) {
    return;
  }

  queue_in_progress_ = true;

  engine_->database()->GetFirstContributionQueue(base::BindOnce(
      &Contribution::OnProcessContributionQueue, weak_factory_.GetWeakPtr()));
}

void Contribution::OnProcessContributionQueue(
    mojom::ContributionQueuePtr info) {
  if (!info) {
    queue_in_progress_ = false;
    return;
  }

  DCHECK(queue_in_progress_);

  Start(std::move(info));
}

void Contribution::CheckNotCompletedContributions() {
  engine_->database()->GetNotCompletedContributions(base::BindOnce(
      &Contribution::NotCompletedContributions, weak_factory_.GetWeakPtr()));
}

void Contribution::NotCompletedContributions(
    std::vector<mojom::ContributionInfoPtr> list) {
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

void Contribution::ResetReconcileStamp() {
  engine_->state()->ResetReconcileStamp();
  SetReconcileStampTimer();
}

void Contribution::StartContributionsForTesting() {
  StartMonthlyContributions(MonthlyContributionOptions::kSendAllContributions);
}

void Contribution::StartMonthlyContributions(
    MonthlyContributionOptions options) {
  if (monthly_contributions_processing_) {
    return;
  }

  monthly_contribution_timer_.Stop();
  monthly_contributions_processing_ = true;

  std::optional<base::Time> cutoff_time = base::Time::Now();

  // Existing tests expect the ability to trigger all monthly and AC
  // contributions, regardless of their "next contribution dates". If we are
  // triggering all contributions, pass a null cutoff time to the monthly
  // contribution processor so that it will send them all.
  if (options == MonthlyContributionOptions::kSendAllContributions) {
    cutoff_time = std::nullopt;
  }

  engine_->Log(FROM_HERE) << "Starting monthly contributions";

  monthly_.Process(cutoff_time,
                   base::BindOnce(&Contribution::OnMonthlyContributionsFinished,
                                  weak_factory_.GetWeakPtr(), options));
}

void Contribution::OnMonthlyContributionsFinished(
    MonthlyContributionOptions options,
    mojom::Result result) {
  monthly_contributions_processing_ = false;

  // Only restart the timer on success. If we were unable to advance the
  // contribution dates and enqueue contributions, then we'll most likely end
  // up in a failure loop if we retry immediately. The timer will be reset
  // when the user creates another monthly contribution or restarts.
  if (result == mojom::Result::OK) {
    SetMonthlyContributionTimer();
  }
}

void Contribution::OnBalance(mojom::ContributionQueuePtr queue,
                             mojom::BalancePtr balance) {
  if (!balance) {
    queue_in_progress_ = false;
    engine_->LogError(FROM_HERE) << "We couldn't get balance from the server";
    if (queue->type == mojom::RewardsType::ONE_TIME_TIP) {
      MarkContributionQueueAsComplete(queue->id, false);
    }
    return;
  }

  Process(std::move(queue), std::move(balance));
}

void Contribution::Start(mojom::ContributionQueuePtr info) {
  CHECK(info);

  if (info->type == mojom::RewardsType::AUTO_CONTRIBUTE) {
    engine_->LogError(FROM_HERE) << "AC is disabled, skipping contribution";
    MarkContributionQueueAsComplete(info->id, false);
    return;
  }

  auto fetch_callback = base::BindOnce(
      &Contribution::OnBalance, weak_factory_.GetWeakPtr(), std::move(info));
  engine_->wallet()->FetchBalance(std::move(fetch_callback));
}

void Contribution::SetReconcileStampTimer() {
  if (reconcile_stamp_timer_.IsRunning()) {
    return;
  }

  uint64_t now = std::time(nullptr);
  uint64_t next_reconcile_stamp = engine_->state()->GetReconcileStamp();

  base::TimeDelta delay;
  if (next_reconcile_stamp > now) {
    delay = base::Seconds(next_reconcile_stamp - now);
  }

  engine_->Log(FROM_HERE) << "Reconcile stamp timer set for " << delay;

  reconcile_stamp_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&Contribution::ResetReconcileStamp,
                     weak_factory_.GetWeakPtr()));
}

void Contribution::SetMonthlyContributionTimer() {
  // Read the next contribution time of the monthly contribution that will
  // run soonest.
  engine_->database()->GetNextMonthlyContributionTime(
      base::BindOnce(&Contribution::OnNextMonthlyContributionTimeRead,
                     weak_factory_.GetWeakPtr()));
}

void Contribution::OnNextMonthlyContributionTimeRead(
    std::optional<base::Time> time) {
  monthly_contribution_timer_.Stop();

  if (!time) {
    engine_->Log(FROM_HERE) << "No monthly contributions found.";
    return;
  }

  base::TimeDelta delay = *time - base::Time::Now();

  monthly_contribution_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&Contribution::StartMonthlyContributions,
                     weak_factory_.GetWeakPtr(),
                     MonthlyContributionOptions::kDefault));

  engine_->Log(FROM_HERE) << "Monthly contribution timer set for " << delay;
}

void Contribution::SendContribution(const std::string& publisher_id,
                                    double amount,
                                    bool set_monthly,
                                    base::OnceCallback<void(bool)> callback) {
  ContributionRequest request(publisher_id, amount, set_monthly,
                              std::move(callback));

  tip_.Process(publisher_id, amount,
               base::BindOnce(&Contribution::OnContributionRequestQueued,
                              weak_factory_.GetWeakPtr(), std::move(request)));
}

void Contribution::OnContributionRequestQueued(
    ContributionRequest request,
    std::optional<std::string> queue_id) {
  if (!queue_id) {
    std::move(request.callback).Run(false);
    return;
  }

  DCHECK(!queue_id->empty());
  requests_.emplace(std::move(*queue_id), std::move(request));
}

void Contribution::OnContributionRequestCompleted(const std::string& queue_id,
                                                  bool success) {
  auto node = requests_.extract(queue_id);
  if (!node) {
    return;
  }

  auto& request = node.mapped();

  // If the contribution was successful and the user has requested that this be
  // a recurring contribution, record the monthly contribution in the database.
  // Optimistically assume that a write failure will not occur. The callback
  // should receive the result of the contribution, regardless of whether this
  // write succeeds or fails.
  if (success && request.set_monthly) {
    engine_->database()->SetMonthlyContribution(
        request.publisher_id, request.amount,
        base::BindOnce(&Contribution::OnMonthlyContributionSet,
                       weak_factory_.GetWeakPtr()));
  }

  std::move(request.callback).Run(success);
}

void Contribution::OnMonthlyContributionSet(bool success) {
  if (success) {
    // After setting a monthly contribution, reset the monthly contribution
    // timer. Note that we do not need to reset the timer when a monthly
    // contribution is deleted. If a deleted contribution was the soonest to
    // execute, then the monthly contribution processor will simply no-op when
    // it runs.
    SetMonthlyContributionTimer();
  }
}

void Contribution::ContributionCompleted(
    mojom::Result result,
    mojom::ContributionInfoPtr contribution) {
  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution is null";
    return;
  }

  engine_->client()->OnReconcileComplete(result, contribution->Clone());

  if (result == mojom::Result::OK) {
    engine_->database()->SaveBalanceReportInfoItem(
        util::GetCurrentMonth(), util::GetCurrentYear(),
        GetReportTypeFromRewardsType(contribution->type), contribution->amount,
        base::DoNothing());
  }

  engine_->database()->UpdateContributionInfoStepAndCount(
      contribution->contribution_id, ConvertResultIntoContributionStep(result),
      -1,
      base::BindOnce(&Contribution::ContributionCompletedSaved,
                     weak_factory_.GetWeakPtr(),
                     contribution->contribution_id));
}

void Contribution::ContributionCompletedSaved(
    const std::string& contribution_id,
    mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Contribution step and count failed";
  }
}

void Contribution::OneTimeTip(const std::string& publisher_key,
                              double amount,
                              ResultCallback callback) {
  auto on_added = [](ResultCallback callback,
                     std::optional<std::string> queue_id) {
    std::move(callback).Run(mojom::Result::OK);
  };

  tip_.Process(publisher_key, amount,
               base::BindOnce(on_added, std::move(callback)));
}

void Contribution::OnMarkContributionQueueAsComplete(mojom::Result result) {
  queue_in_progress_ = false;
  CheckContributionQueue();
}

void Contribution::MarkContributionQueueAsComplete(const std::string& id,
                                                   bool success) {
  if (id.empty()) {
    engine_->LogError(FROM_HERE) << "Queue id is empty";
    return;
  }

  // If the engine could not successfully create contribution entries for this
  // queue item, then inform any pending callbacks of failure.
  if (!success) {
    OnContributionRequestCompleted(id, false);
  }

  engine_->database()->MarkContributionQueueAsComplete(
      id, base::BindOnce(&Contribution::OnMarkContributionQueueAsComplete,
                         weak_factory_.GetWeakPtr()));
}

void Contribution::CreateNewEntry(const std::string& wallet_type,
                                  mojom::BalancePtr balance,
                                  mojom::ContributionQueuePtr queue) {
  if (!queue) {
    engine_->Log(FROM_HERE) << "Queue is null";
    return;
  }

  if (queue->publishers.empty() || !balance || wallet_type.empty()) {
    engine_->LogError(FROM_HERE) << "Queue data is wrong";
    MarkContributionQueueAsComplete(queue->id, false);
    return;
  }

  if (queue->type == mojom::RewardsType::AUTO_CONTRIBUTE) {
    engine_->LogError(FROM_HERE) << "AC is disabled";
    MarkContributionQueueAsComplete(queue->id, false);
    return;
  }

  const auto balance_it = balance->wallets.find(wallet_type);
  const double wallet_balance =
      balance_it != balance->wallets.cend() ? balance_it->second : 0.0;
  if (wallet_balance == 0) {
    engine_->Log(FROM_HERE) << "Wallet balance is 0 for " << wallet_type;
    CreateNewEntry(GetNextProcessor(wallet_type), std::move(balance),
                   std::move(queue));
    return;
  }

  const std::string contribution_id =
      base::Uuid::GenerateRandomV4().AsLowercaseString();

  auto contribution = mojom::ContributionInfo::New();
  const uint64_t now = util::GetCurrentTimeStamp();
  contribution->contribution_id = contribution_id;
  contribution->amount = queue->amount;
  contribution->type = queue->type;
  contribution->step = mojom::ContributionStep::STEP_START;
  contribution->retry_count = 0;
  contribution->created_at = now;
  contribution->processor = GetProcessor(wallet_type);

  std::vector<mojom::ContributionQueuePublisherPtr> queue_publishers;
  for (auto& item : queue->publishers) {
    queue_publishers.push_back(item->Clone());
  }

  if (wallet_balance < queue->amount) {
    contribution->amount = wallet_balance;
    queue->amount = queue->amount - wallet_balance;
  } else {
    queue->amount = 0;
  }

  engine_->Log(FROM_HERE) << "Creating contribution for wallet type "
                          << wallet_type << " (amount: " << contribution->amount
                          << ", type: " << queue->type << ")";

  std::vector<mojom::ContributionPublisherPtr> publisher_list;
  for (const auto& item : queue_publishers) {
    auto publisher = mojom::ContributionPublisher::New();
    publisher->contribution_id = contribution_id;
    publisher->publisher_key = item->publisher_key;
    publisher->total_amount =
        (item->amount_percent * contribution->amount) / 100;
    publisher->contributed_amount = 0;
    publisher_list.push_back(std::move(publisher));
  }

  contribution->publishers = std::move(publisher_list);

  engine_->database()->SaveContributionInfo(
      contribution->Clone(),
      base::BindOnce(&Contribution::OnEntrySaved, weak_factory_.GetWeakPtr(),
                     contribution->contribution_id, wallet_type,
                     std::move(balance), std::move(queue)));
}

void Contribution::OnEntrySaved(const std::string& contribution_id,
                                const std::string& wallet_type,
                                mojom::BalancePtr balance,
                                mojom::ContributionQueuePtr queue,
                                mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Contribution was not saved correctly";
    return;
  }

  if (!queue) {
    engine_->LogError(FROM_HERE) << "Queue is null";
    return;
  }

  const std::string& queue_id = queue->id;

  if (wallet_type == constant::kWalletUphold ||
      wallet_type == constant::kWalletBitflyer ||
      wallet_type == constant::kWalletGemini) {
    external_wallet_.Process(
        contribution_id,
        base::BindOnce(&Contribution::Result, weak_factory_.GetWeakPtr(),
                       queue_id, contribution_id));
  }

  if (queue->amount > 0) {
    auto save_callback =
        base::BindOnce(&Contribution::OnQueueSaved, weak_factory_.GetWeakPtr(),
                       wallet_type, std::move(balance), queue->Clone());

    engine_->database()->SaveContributionQueue(std::move(queue),
                                               std::move(save_callback));
  } else {
    MarkContributionQueueAsComplete(queue->id, true);
  }
}

void Contribution::OnQueueSaved(const std::string& wallet_type,
                                mojom::BalancePtr balance,
                                mojom::ContributionQueuePtr queue,
                                mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Queue was not saved successfully";
    return;
  }

  if (!queue) {
    engine_->LogError(FROM_HERE) << "Queue was not converted successfully";
    return;
  }

  CreateNewEntry(GetNextProcessor(wallet_type), std::move(balance),
                 std::move(queue));
}

void Contribution::Process(mojom::ContributionQueuePtr queue,
                           mojom::BalancePtr balance) {
  if (!queue) {
    engine_->LogError(FROM_HERE) << "Queue is null";
    return;
  }

  if (!balance) {
    engine_->LogError(FROM_HERE) << "Balance is null";
    return;
  }

  if (queue->amount == 0 || queue->publishers.empty()) {
    engine_->LogError(FROM_HERE) << "Amount/publisher is empty";
    MarkContributionQueueAsComplete(queue->id, false);
    return;
  }

  const auto have_enough_balance = HaveEnoughFundsToContribute(
      &queue->amount, queue->partial, balance->total);

  if (!have_enough_balance) {
    engine_->Log(FROM_HERE) << "Not enough balance";
    MarkContributionQueueAsComplete(queue->id, false);
    return;
  }

  if (queue->amount == 0) {
    engine_->LogError(FROM_HERE) << "Amount is 0";
    MarkContributionQueueAsComplete(queue->id, false);
    return;
  }

  CreateNewEntry(GetNextProcessor(""), std::move(balance), std::move(queue));
}

void Contribution::Result(const std::string& queue_id,
                          const std::string& contribution_id,
                          mojom::Result result) {
  engine_->database()->GetContributionInfo(
      contribution_id,
      base::BindOnce(&Contribution::OnResult, weak_factory_.GetWeakPtr(),
                     result, queue_id));
}

void Contribution::OnResult(mojom::Result result,
                            const std::string& queue_id,
                            mojom::ContributionInfoPtr contribution) {
  // Notify any waiting callbacks that the contribution request has either
  // succeeded or failed. Note that if the contribution was "split" between
  // multiple funding sources, then the callback will only receive the
  // completion status for the first completed transaction. In addition, a
  // successful completion status only indicates that the transaction was
  // successfully initiated. There may be a significant delay before the
  // transaction is completed by the provider.
  OnContributionRequestCompleted(
      queue_id, result == mojom::Result::OK ||
                    result == mojom::Result::RETRY_PENDING_TRANSACTION_SHORT ||
                    result == mojom::Result::RETRY_PENDING_TRANSACTION_LONG);

  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution is null";
    return;
  }

  if (result == mojom::Result::RETRY_PENDING_TRANSACTION_SHORT) {
    SetRetryTimer(contribution->contribution_id, base::Seconds(10));
    return;
  }

  if (result == mojom::Result::RETRY_PENDING_TRANSACTION_LONG) {
    SetRetryTimer(contribution->contribution_id, base::Minutes(5));
    return;
  }

  if (result == mojom::Result::RETRY_SHORT) {
    SetRetryTimer(contribution->contribution_id, base::Seconds(5));
    return;
  }

  if (result == mojom::Result::RETRY_LONG) {
    if (contribution->processor == mojom::ContributionProcessor::BRAVE_TOKENS) {
      SetRetryTimer(contribution->contribution_id,
                    util::GetRandomizedDelay(base::Seconds(45)));
    } else {
      SetRetryTimer(contribution->contribution_id,
                    util::GetRandomizedDelay(base::Seconds(450)));
    }
    return;
  }

  if (result == mojom::Result::RETRY) {
    SetRetryTimer(contribution->contribution_id,
                  util::GetRandomizedDelay(base::Seconds(45)));
    return;
  }

  ContributionCompleted(result, std::move(contribution));
}

void Contribution::SetRetryTimer(const std::string& contribution_id,
                                 base::TimeDelta delay) {
  if (contribution_id.empty()) {
    engine_->LogError(FROM_HERE) << "Contribution id is empty";
    return;
  }

  if (engine_->options().retry_interval) {
    delay = base::Seconds(engine_->options().retry_interval);
  }

  engine_->Log(FROM_HERE) << "Timer for contribution retry (" << contribution_id
                          << ") "
                             "set for "
                          << delay;

  retry_timers_[contribution_id].Start(
      FROM_HERE, delay,
      base::BindOnce(&Contribution::OnRetryTimerElapsed,
                     weak_factory_.GetWeakPtr(), contribution_id));
}

void Contribution::OnRetryTimerElapsed(const std::string& contribution_id) {
  retry_timers_.erase(contribution_id);

  engine_->database()->GetContributionInfo(
      contribution_id, base::BindOnce(&Contribution::SetRetryCounter,
                                      weak_factory_.GetWeakPtr()));
}

void Contribution::SetRetryCounter(mojom::ContributionInfoPtr contribution) {
  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution is null";
    return;
  }

  if (contribution->retry_count == 5 &&
      contribution->step != mojom::ContributionStep::STEP_PREPARE) {
    engine_->LogError(FROM_HERE) << "Contribution failed after 5 retries";
    ContributionCompleted(mojom::Result::TOO_MANY_RESULTS,
                          std::move(contribution));
    return;
  }

  engine_->database()->UpdateContributionInfoStepAndCount(
      contribution->contribution_id, contribution->step,
      contribution->retry_count + 1,
      base::BindOnce(&Contribution::Retry, weak_factory_.GetWeakPtr(),
                     contribution->Clone()));
}

void Contribution::Retry(mojom::ContributionInfoPtr contribution,
                         mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Retry count update failed";
    return;
  }

  if (!contribution) {
    engine_->LogError(FROM_HERE) << "Contribution is null";
    return;
  }

  // negative steps are final steps, nothing to retry
  if (static_cast<int>(contribution->step) < 0) {
    return;
  }

  if (contribution->type == mojom::RewardsType::AUTO_CONTRIBUTE) {
    engine_->Log(FROM_HERE) << "AC is disabled, completing contribution";
    ContributionCompleted(mojom::Result::FAILED, std::move(contribution));
    return;
  }

  engine_->Log(FROM_HERE) << "Retrying contribution ("
                          << contribution->contribution_id << ") on step "
                          << contribution->step;

  auto result_callback =
      base::BindOnce(&Contribution::Result, weak_factory_.GetWeakPtr(), "",
                     contribution->contribution_id);

  switch (contribution->processor) {
    case mojom::ContributionProcessor::BRAVE_TOKENS: {
      Result("", contribution->contribution_id, mojom::Result::FAILED);
      return;
    }
    case mojom::ContributionProcessor::UPHOLD:
    case mojom::ContributionProcessor::BITFLYER:
    case mojom::ContributionProcessor::GEMINI: {
      external_wallet_.Retry(std::move(contribution),
                             std::move(result_callback));
      return;
    }
    case mojom::ContributionProcessor::NONE: {
      Result("", contribution->contribution_id, mojom::Result::FAILED);
      return;
    }
  }
}

void Contribution::GetRecurringTips(GetRecurringTipsCallback callback) {
  engine_->database()->GetRecurringTips(
      base::BindOnce(&Contribution::OnRecurringTipsRead,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void Contribution::OnRecurringTipsRead(
    GetRecurringTipsCallback callback,
    std::vector<mojom::PublisherInfoPtr> list) {
  // The publisher status field may be expired. Attempt to refresh
  // expired publisher status values before executing callback.
  publisher::RefreshPublisherStatus(*engine_, std::move(list),
                                    std::move(callback));
}

}  // namespace contribution
}  // namespace brave_rewards::internal
