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
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "brave/components/brave_rewards/core/uphold/uphold.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "brave/components/brave_rewards/core/wallet/wallet_balance.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace brave_rewards::internal {

namespace {

bool IsRevivedAC(const mojom::ContributionInfo& contribution) {
  using mojom::ContributionProcessor;
  using mojom::RewardsType;

  if (contribution.type != RewardsType::AUTO_CONTRIBUTE) {
    return false;
  }

  // Only externally-funded ACs are considered "revived".
  if (contribution.processor == ContributionProcessor::BRAVE_TOKENS) {
    return false;
  }

  // If the |created_at| field does not have a valid timestamp, then do not
  // consider this a revived AC.
  if (contribution.created_at == 0) {
    return false;
  }

  // ACs that were started over 20 days ago are considered "revived".
  base::Time created_at =
      base::Time::FromSecondsSinceUnixEpoch(contribution.created_at);
  return base::Time::Now() - created_at > base::Days(20);
}

mojom::ContributionStep ConvertResultIntoContributionStep(
    const mojom::Result result) {
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

Contribution::Contribution(RewardsEngineImpl& engine)
    : engine_(engine),
      unblinded_(engine),
      sku_(engine),
      monthly_(engine),
      ac_(engine),
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

  BLOG(1, "Queue timer set for " << delay);

  queue_timer_.Start(FROM_HERE, delay,
                     base::BindOnce(&Contribution::ProcessContributionQueue,
                                    base::Unretained(this)));
}

void Contribution::ProcessContributionQueue() {
  if (queue_in_progress_) {
    return;
  }

  queue_in_progress_ = true;

  const auto callback =
      std::bind(&Contribution::OnProcessContributionQueue, this, _1);
  engine_->database()->GetFirstContributionQueue(callback);
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
  auto get_callback =
      std::bind(&Contribution::NotCompletedContributions, this, _1);

  engine_->database()->GetNotCompletedContributions(get_callback);
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
  SetAutoContributeTimer();
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

  BLOG(1, "Starting monthly contributions");

  monthly_.Process(cutoff_time, [this, options](mojom::Result result) {
    monthly_contributions_processing_ = false;

    // Only restart the timer on success. If we were unable to advance the
    // contribution dates and enqueue contributions, then we'll most likely end
    // up in a failure loop if we retry immediately. The timer will be reset
    // when the user creates another monthly contribution or restarts.
    if (result == mojom::Result::OK) {
      SetMonthlyContributionTimer();
    }

    // Existing tests expect that, when all contributions are triggered, monthly
    // contributions will be sent before AC contributions. When testing, we
    // process contributions in series instead of in parallel.
    if (options == MonthlyContributionOptions::kSendAllContributions) {
      StartAutoContribute();
    }
  });
}

void Contribution::StartAutoContribute() {
  uint64_t reconcile_stamp = engine_->state()->GetReconcileStamp();
  ResetReconcileStamp();
  BLOG(1, "Starting auto-contribute");
  ac_.Process(reconcile_stamp);
}

void Contribution::OnBalance(mojom::ContributionQueuePtr queue,
                             mojom::BalancePtr balance) {
  if (!balance) {
    queue_in_progress_ = false;
    BLOG(0, "We couldn't get balance from the server.");
    if (queue->type == mojom::RewardsType::ONE_TIME_TIP) {
      MarkContributionQueueAsComplete(queue->id, false);
    }
    return;
  }

  Process(std::move(queue), std::move(balance));
}

void Contribution::Start(mojom::ContributionQueuePtr info) {
  auto fetch_callback = base::BindOnce(&Contribution::OnBalance,
                                       base::Unretained(this), std::move(info));
  engine_->wallet()->FetchBalance(std::move(fetch_callback));
}

void Contribution::SetAutoContributeTimer() {
  if (auto_contribute_timer_.IsRunning()) {
    return;
  }

  uint64_t now = std::time(nullptr);
  uint64_t next_reconcile_stamp = engine_->state()->GetReconcileStamp();

  base::TimeDelta delay;
  if (next_reconcile_stamp > now) {
    delay = base::Seconds(next_reconcile_stamp - now);
  }

  BLOG(1, "Auto-contribute timer set for " << delay);

  auto_contribute_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&Contribution::StartAutoContribute,
                     base::Unretained(this)));
}

void Contribution::SetMonthlyContributionTimer() {
  // Read the next contribution time of the monthly contribution that will
  // run soonest.
  engine_->database()->GetNextMonthlyContributionTime(
      base::BindOnce(&Contribution::OnNextMonthlyContributionTimeRead,
                     base::Unretained(this)));
}

void Contribution::OnNextMonthlyContributionTimeRead(
    std::optional<base::Time> time) {
  monthly_contribution_timer_.Stop();

  if (!time) {
    BLOG(1, "No monthly contributions found.");
    return;
  }

  base::TimeDelta delay = *time - base::Time::Now();

  monthly_contribution_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&Contribution::StartMonthlyContributions,
                     base::Unretained(this),
                     MonthlyContributionOptions::kDefault));

  BLOG(1, "Monthly contribution timer set for " << delay);
}

void Contribution::SendContribution(const std::string& publisher_id,
                                    double amount,
                                    bool set_monthly,
                                    base::OnceCallback<void(bool)> callback) {
  ContributionRequest request(publisher_id, amount, set_monthly,
                              std::move(callback));

  tip_.Process(publisher_id, amount,
               base::BindOnce(&Contribution::OnContributionRequestQueued,
                              base::Unretained(this), std::move(request)));
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
                       base::Unretained(this)));
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
    const mojom::Result result,
    mojom::ContributionInfoPtr contribution) {
  if (!contribution) {
    BLOG(0, "Contribution is null");
    return;
  }

  // It is currently possible for some externally funded ACs to be stalled until
  // browser restart. Those ACs should complete in the background without
  // updating the current month's balance report or generating a notification.
  if (!IsRevivedAC(*contribution)) {
    engine_->client()->OnReconcileComplete(result, contribution->Clone());

    if (result == mojom::Result::OK) {
      engine_->database()->SaveBalanceReportInfoItem(
          util::GetCurrentMonth(), util::GetCurrentYear(),
          GetReportTypeFromRewardsType(contribution->type),
          contribution->amount, [](const mojom::Result) {});
    }
  }

  auto save_callback = std::bind(&Contribution::ContributionCompletedSaved,
                                 this, _1, contribution->contribution_id);

  engine_->database()->UpdateContributionInfoStepAndCount(
      contribution->contribution_id, ConvertResultIntoContributionStep(result),
      -1, save_callback);
}

void Contribution::ContributionCompletedSaved(
    const mojom::Result result,
    const std::string& contribution_id) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Contribution step and count failed");
  }

  auto callback = std::bind(&Contribution::OnMarkUnblindedTokensAsSpendable,
                            this, _1, contribution_id);
  engine_->database()->MarkUnblindedTokensAsSpendable(contribution_id,
                                                      callback);
}

void Contribution::OneTimeTip(const std::string& publisher_key,
                              double amount,
                              LegacyResultCallback callback) {
  auto on_added = [](LegacyResultCallback callback,
                     std::optional<std::string> queue_id) {
    callback(mojom::Result::OK);
  };

  tip_.Process(publisher_key, amount,
               base::BindOnce(on_added, std::move(callback)));
}

void Contribution::OnMarkContributionQueueAsComplete(
    const mojom::Result result) {
  queue_in_progress_ = false;
  CheckContributionQueue();
}

void Contribution::MarkContributionQueueAsComplete(const std::string& id,
                                                   bool success) {
  if (id.empty()) {
    BLOG(0, "Queue id is empty");
    return;
  }

  // If the engine could not successfully create contribution entries for this
  // queue item, then inform any pending callbacks of failure.
  if (!success) {
    OnContributionRequestCompleted(id, false);
  }

  auto callback =
      std::bind(&Contribution::OnMarkContributionQueueAsComplete, this, _1);

  engine_->database()->MarkContributionQueueAsComplete(id, callback);
}

void Contribution::CreateNewEntry(const std::string& wallet_type,
                                  mojom::BalancePtr balance,
                                  mojom::ContributionQueuePtr queue) {
  if (!queue) {
    BLOG(1, "Queue is null");
    return;
  }

  if (queue->publishers.empty() || !balance || wallet_type.empty()) {
    BLOG(0, "Queue data is wrong");
    MarkContributionQueueAsComplete(queue->id, false);
    return;
  }

  const auto balance_it = balance->wallets.find(wallet_type);
  const double wallet_balance =
      balance_it != balance->wallets.cend() ? balance_it->second : 0.0;
  if (wallet_balance == 0) {
    BLOG(1, "Wallet balance is 0 for " << wallet_type);
    CreateNewEntry(GetNextProcessor(wallet_type), std::move(balance),
                   std::move(queue));
    return;
  }

  if (wallet_type == constant::kWalletBitflyer &&
      queue->type == mojom::RewardsType::AUTO_CONTRIBUTE) {
    BLOG(1, "AC is not supported for bitFlyer wallets");
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

  BLOG(1, "Creating contribution for wallet type "
              << wallet_type << " (amount: " << contribution->amount
              << ", type: " << queue->type << ")");

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

  auto shared_queue =
      std::make_shared<mojom::ContributionQueuePtr>(std::move(queue));

  auto save_callback = std::bind(&Contribution::OnEntrySaved, this, _1,
                                 contribution->contribution_id, wallet_type,
                                 *balance, shared_queue);

  engine_->database()->SaveContributionInfo(contribution->Clone(),
                                            save_callback);
}

void Contribution::OnEntrySaved(
    const mojom::Result result,
    const std::string& contribution_id,
    const std::string& wallet_type,
    const mojom::Balance& balance,
    std::shared_ptr<mojom::ContributionQueuePtr> shared_queue) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Contribution was not saved correctly");
    return;
  }

  if (!shared_queue) {
    BLOG(0, "Queue is null");
    return;
  }

  const std::string& queue_id = (*shared_queue)->id;

  if (wallet_type == constant::kWalletUnBlinded) {
    auto result_callback =
        std::bind(&Contribution::Result, this, _1, queue_id, contribution_id);

    StartUnblinded({mojom::CredsBatchType::PROMOTION}, contribution_id,
                   result_callback);
  } else if (wallet_type == constant::kWalletUphold ||
             wallet_type == constant::kWalletBitflyer ||
             wallet_type == constant::kWalletGemini) {
    auto result_callback =
        std::bind(&Contribution::Result, this, _1, queue_id, contribution_id);

    external_wallet_.Process(contribution_id, result_callback);
  }

  if ((*shared_queue)->amount > 0) {
    auto save_callback = std::bind(&Contribution::OnQueueSaved, this, _1,
                                   wallet_type, balance, shared_queue);

    engine_->database()->SaveContributionQueue((*shared_queue)->Clone(),
                                               save_callback);
  } else {
    MarkContributionQueueAsComplete((*shared_queue)->id, true);
  }
}

void Contribution::OnQueueSaved(
    const mojom::Result result,
    const std::string& wallet_type,
    const mojom::Balance& balance,
    std::shared_ptr<mojom::ContributionQueuePtr> shared_queue) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Queue was not saved successfully");
    return;
  }

  if (!shared_queue) {
    BLOG(0, "Queue was not converted successfully");
    return;
  }

  CreateNewEntry(GetNextProcessor(wallet_type), mojom::Balance::New(balance),
                 std::move(*shared_queue));
}

void Contribution::Process(mojom::ContributionQueuePtr queue,
                           mojom::BalancePtr balance) {
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
    MarkContributionQueueAsComplete(queue->id, false);
    return;
  }

  const auto have_enough_balance = HaveEnoughFundsToContribute(
      &queue->amount, queue->partial, balance->total);

  if (!have_enough_balance) {
    BLOG(1, "Not enough balance");
    MarkContributionQueueAsComplete(queue->id, false);
    return;
  }

  if (queue->amount == 0) {
    BLOG(0, "Amount is 0");
    MarkContributionQueueAsComplete(queue->id, false);
    return;
  }

  CreateNewEntry(GetNextProcessor(""), balance->Clone(), queue->Clone());
}

void Contribution::TransferFunds(const mojom::SKUTransaction& transaction,
                                 const std::string& destination,
                                 const std::string& wallet_type,
                                 const std::string& contribution_id,
                                 LegacyResultCallback callback) {
  if (wallet_type == constant::kWalletUphold) {
    engine_->uphold()->TransferFunds(transaction.amount, destination,
                                     contribution_id, callback);
    return;
  }

  if (wallet_type == constant::kWalletBitflyer) {
    engine_->bitflyer()->TransferFunds(transaction.amount, destination,
                                       contribution_id, callback);
    return;
  }

  if (wallet_type == constant::kWalletGemini) {
    engine_->gemini()->TransferFunds(transaction.amount, destination,
                                     contribution_id, callback);
    return;
  }

  if (wallet_type == constant::kWalletUnBlinded) {
    sku_.Merchant(transaction, callback);
    return;
  }

  NOTREACHED();
  BLOG(0, "Wallet type not supported: " << wallet_type);
}

void Contribution::SKUAutoContribution(const std::string& contribution_id,
                                       const std::string& wallet_type,
                                       LegacyResultCallback callback) {
  sku_.AutoContribution(contribution_id, wallet_type, callback);
}

void Contribution::StartUnblinded(
    const std::vector<mojom::CredsBatchType>& types,
    const std::string& contribution_id,
    LegacyResultCallback callback) {
  unblinded_.Start(types, contribution_id, callback);
}

void Contribution::RetryUnblinded(
    const std::vector<mojom::CredsBatchType>& types,
    const std::string& contribution_id,
    LegacyResultCallback callback) {
  auto get_callback = std::bind(&Contribution::RetryUnblindedContribution, this,
                                _1, types, callback);

  engine_->database()->GetContributionInfo(contribution_id, get_callback);
}

void Contribution::RetryUnblindedContribution(
    mojom::ContributionInfoPtr contribution,
    const std::vector<mojom::CredsBatchType>& types,
    LegacyResultCallback callback) {
  unblinded_.Retry(types, std::move(contribution), callback);
}

void Contribution::Result(const mojom::Result result,
                          const std::string& queue_id,
                          const std::string& contribution_id) {
  auto get_callback =
      std::bind(&Contribution::OnResult, this, _1, result, queue_id);

  engine_->database()->GetContributionInfo(contribution_id, get_callback);
}

void Contribution::OnResult(mojom::ContributionInfoPtr contribution,
                            const mojom::Result result,
                            const std::string& queue_id) {
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
    BLOG(0, "Contribution is null");
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
    BLOG(0, "Contribution id is empty");
    return;
  }

  if (engine_->options().retry_interval) {
    delay = base::Seconds(engine_->options().retry_interval);
  }

  BLOG(1, "Timer for contribution retry (" << contribution_id
                                           << ") "
                                              "set for "
                                           << delay);

  retry_timers_[contribution_id].Start(
      FROM_HERE, delay,
      base::BindOnce(&Contribution::OnRetryTimerElapsed, base::Unretained(this),
                     contribution_id));
}

void Contribution::OnRetryTimerElapsed(const std::string& contribution_id) {
  retry_timers_.erase(contribution_id);

  auto callback = std::bind(&Contribution::SetRetryCounter, this, _1);

  engine_->database()->GetContributionInfo(contribution_id, callback);
}

void Contribution::SetRetryCounter(mojom::ContributionInfoPtr contribution) {
  if (!contribution) {
    BLOG(0, "Contribution is null");
    return;
  }

  if (contribution->retry_count == 5 &&
      contribution->step != mojom::ContributionStep::STEP_PREPARE) {
    BLOG(0, "Contribution failed after 5 retries");
    ContributionCompleted(mojom::Result::TOO_MANY_RESULTS,
                          std::move(contribution));
    return;
  }

  auto save_callback = std::bind(
      &Contribution::Retry, this, _1,
      std::make_shared<mojom::ContributionInfoPtr>(contribution->Clone()));

  engine_->database()->UpdateContributionInfoStepAndCount(
      contribution->contribution_id, contribution->step,
      contribution->retry_count + 1, save_callback);
}

void Contribution::OnMarkUnblindedTokensAsSpendable(
    const mojom::Result result,
    const std::string& contribution_id) {
  BLOG_IF(1, result != mojom::Result::OK,
          "Failed to mark unblinded tokens as unreserved for contribution "
              << contribution_id);
}

void Contribution::Retry(
    const mojom::Result result,
    std::shared_ptr<mojom::ContributionInfoPtr> shared_contribution) {
  if (result != mojom::Result::OK) {
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

  if ((*shared_contribution)->type == mojom::RewardsType::AUTO_CONTRIBUTE &&
      !engine_->state()->GetAutoContributeEnabled()) {
    BLOG(1, "AC is disabled, completing contribution");
    ContributionCompleted(mojom::Result::AC_OFF,
                          std::move(*shared_contribution));
    return;
  }

  BLOG(1, "Retrying contribution (" << (*shared_contribution)->contribution_id
                                    << ") on step "
                                    << (*shared_contribution)->step);

  auto result_callback = std::bind(&Contribution::Result, this, _1, "",
                                   (*shared_contribution)->contribution_id);

  switch ((*shared_contribution)->processor) {
    case mojom::ContributionProcessor::BRAVE_TOKENS: {
      RetryUnblindedContribution((*shared_contribution)->Clone(),
                                 {mojom::CredsBatchType::PROMOTION},
                                 result_callback);
      return;
    }
    case mojom::ContributionProcessor::UPHOLD:
    case mojom::ContributionProcessor::BITFLYER:
    case mojom::ContributionProcessor::GEMINI: {
      if ((*shared_contribution)->type == mojom::RewardsType::AUTO_CONTRIBUTE) {
        sku_.Retry((*shared_contribution)->Clone(), result_callback);
        return;
      }

      external_wallet_.Retry((*shared_contribution)->Clone(), result_callback);
      return;
    }
    case mojom::ContributionProcessor::NONE: {
      Result(mojom::Result::FAILED, "",
             (*shared_contribution)->contribution_id);
      return;
    }
  }
}

void Contribution::GetRecurringTips(GetRecurringTipsCallback callback) {
  engine_->database()->GetRecurringTips(
      [this, callback](std::vector<mojom::PublisherInfoPtr> list) {
        // The publisher status field may be expired. Attempt to refresh
        // expired publisher status values before executing callback.
        publisher::RefreshPublisherStatus(*engine_, std::move(list), callback);
      });
}

}  // namespace contribution
}  // namespace brave_rewards::internal
