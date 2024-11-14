/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_core.mojom.h"
#include "brave/components/brave_rewards/core/contribution/contribution_external_wallet.h"
#include "brave/components/brave_rewards/core/contribution/contribution_monthly.h"
#include "brave/components/brave_rewards/core/contribution/contribution_tip.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace contribution {

class Contribution {
 public:
  explicit Contribution(RewardsEngine& engine);

  ~Contribution();

  void Initialize();

  void ProcessContributionQueue();

  void SetReconcileStampTimer();

  void SetMonthlyContributionTimer();

  void StartContributionsForTesting();

  void SendContribution(const std::string& publisher_id,
                        double amount,
                        bool set_monthly,
                        base::OnceCallback<void(bool)> callback);

  void ContributionCompleted(mojom::Result result,
                             mojom::ContributionInfoPtr contribution);

  void ResetReconcileStamp();

  void OneTimeTip(const std::string& publisher_key,
                  double amount,
                  ResultCallback callback);

  void CheckContributionQueue();

  void GetRecurringTips(GetRecurringTipsCallback callback);

 private:
  enum class MonthlyContributionOptions { kDefault, kSendAllContributions };

  void StartMonthlyContributions(MonthlyContributionOptions options);

  void OnMonthlyContributionsFinished(MonthlyContributionOptions options,
                                      mojom::Result result);

  void OnNextMonthlyContributionTimeRead(std::optional<base::Time> time);

  void OnMonthlyContributionSet(bool success);

  struct ContributionRequest {
    ContributionRequest(std::string publisher_id,
                        double amount,
                        bool set_monthly,
                        base::OnceCallback<void(bool)> callback);

    ContributionRequest(ContributionRequest&& other);
    ContributionRequest& operator=(ContributionRequest&& other);

    ~ContributionRequest();

    std::string publisher_id;
    double amount;
    bool set_monthly;
    base::OnceCallback<void(bool)> callback;
  };

  void OnContributionRequestQueued(ContributionRequest request,
                                   std::optional<std::string> queue_id);

  void OnContributionRequestCompleted(const std::string& queue_id,
                                      bool success);

  // Start point for contribution
  // In this step we get balance from the server
  void Start(mojom::ContributionQueuePtr info);

  void ContributionCompletedSaved(const std::string& contribution_id,
                                  mojom::Result result);

  void OnProcessContributionQueue(mojom::ContributionQueuePtr info);

  void CheckNotCompletedContributions();

  void NotCompletedContributions(std::vector<mojom::ContributionInfoPtr> list);

  void OnBalance(mojom::ContributionQueuePtr queue, mojom::BalancePtr balance);

  void CreateNewEntry(const std::string& wallet_type,
                      mojom::BalancePtr balance,
                      mojom::ContributionQueuePtr queue);

  void OnEntrySaved(const std::string& contribution_id,
                    const std::string& wallet_type,
                    mojom::BalancePtr balance,
                    mojom::ContributionQueuePtr queue,
                    mojom::Result result);

  void OnQueueSaved(const std::string& wallet_type,
                    mojom::BalancePtr balance,
                    mojom::ContributionQueuePtr queue,
                    mojom::Result result);

  void Process(mojom::ContributionQueuePtr queue, mojom::BalancePtr balance);

  void MarkContributionQueueAsComplete(const std::string& id, bool success);

  void OnMarkContributionQueueAsComplete(mojom::Result result);

  void Result(const std::string& queue_id,
              const std::string& contribution_id,
              mojom::Result result);

  void OnResult(mojom::Result result,
                const std::string& queue_id,
                mojom::ContributionInfoPtr contribution);

  void SetRetryTimer(const std::string& contribution_id, base::TimeDelta delay);

  void OnRetryTimerElapsed(const std::string& contribution_id);

  void SetRetryCounter(mojom::ContributionInfoPtr contribution);

  void Retry(mojom::ContributionInfoPtr contribution, mojom::Result result);

  void OnRecurringTipsRead(GetRecurringTipsCallback callback,
                           std::vector<mojom::PublisherInfoPtr> list);

  const raw_ref<RewardsEngine> engine_;
  ContributionMonthly monthly_;
  ContributionTip tip_;
  ContributionExternalWallet external_wallet_;
  std::map<std::string, ContributionRequest> requests_;
  base::OneShotTimer reconcile_stamp_timer_;
  base::OneShotTimer monthly_contribution_timer_;
  std::map<std::string, base::OneShotTimer> retry_timers_;
  base::OneShotTimer queue_timer_;
  bool queue_in_progress_ = false;
  bool monthly_contributions_processing_ = false;

  base::WeakPtrFactory<Contribution> weak_factory_{this};
};

}  // namespace contribution
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_H_
