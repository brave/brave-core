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
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_rewards/core/contribution/contribution_ac.h"
#include "brave/components/brave_rewards/core/contribution/contribution_external_wallet.h"
#include "brave/components/brave_rewards/core/contribution/contribution_monthly.h"
#include "brave/components/brave_rewards/core/contribution/contribution_sku.h"
#include "brave/components/brave_rewards/core/contribution/contribution_tip.h"
#include "brave/components/brave_rewards/core/contribution/contribution_unblinded.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace contribution {

class Contribution {
 public:
  explicit Contribution(RewardsEngineImpl& engine);

  ~Contribution();

  void Initialize();

  void ProcessContributionQueue();

  void SetAutoContributeTimer();

  void SetMonthlyContributionTimer();

  void StartContributionsForTesting();

  void SendContribution(const std::string& publisher_id,
                        double amount,
                        bool set_monthly,
                        base::OnceCallback<void(bool)> callback);

  void ContributionCompleted(const mojom::Result result,
                             mojom::ContributionInfoPtr contribution);

  void ResetReconcileStamp();

  void OneTimeTip(const std::string& publisher_key,
                  double amount,
                  LegacyResultCallback callback);

  void CheckContributionQueue();

  void TransferFunds(const mojom::SKUTransaction& transaction,
                     const std::string& destination,
                     const std::string& wallet_type,
                     const std::string& contribution_id,
                     LegacyResultCallback callback);

  void SKUAutoContribution(const std::string& contribution_id,
                           const std::string& wallet_type,
                           LegacyResultCallback callback);

  void StartUnblinded(const std::vector<mojom::CredsBatchType>& types,
                      const std::string& contribution_id,
                      LegacyResultCallback callback);

  void RetryUnblinded(const std::vector<mojom::CredsBatchType>& types,
                      const std::string& contribution_id,
                      LegacyResultCallback callback);

  void GetRecurringTips(GetRecurringTipsCallback callback);

 private:
  enum class MonthlyContributionOptions { kDefault, kSendAllContributions };

  void StartMonthlyContributions(MonthlyContributionOptions options);

  void StartAutoContribute();

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

  void ContributionCompletedSaved(const mojom::Result result,
                                  const std::string& contribution_id);

  void OnProcessContributionQueue(mojom::ContributionQueuePtr info);

  void CheckNotCompletedContributions();

  void NotCompletedContributions(std::vector<mojom::ContributionInfoPtr> list);

  void OnBalance(mojom::ContributionQueuePtr queue, mojom::BalancePtr balance);

  void CreateNewEntry(const std::string& wallet_type,
                      mojom::BalancePtr balance,
                      mojom::ContributionQueuePtr queue);

  void OnEntrySaved(const mojom::Result result,
                    const std::string& contribution_id,
                    const std::string& wallet_type,
                    const mojom::Balance& balance,
                    std::shared_ptr<mojom::ContributionQueuePtr> shared_queue);

  void OnQueueSaved(const mojom::Result result,
                    const std::string& wallet_type,
                    const mojom::Balance& balance,
                    std::shared_ptr<mojom::ContributionQueuePtr> shared_queue);

  void Process(mojom::ContributionQueuePtr queue, mojom::BalancePtr balance);

  void MarkContributionQueueAsComplete(const std::string& id, bool success);

  void OnMarkContributionQueueAsComplete(const mojom::Result result);

  void RetryUnblindedContribution(
      mojom::ContributionInfoPtr contribution,
      const std::vector<mojom::CredsBatchType>& types,
      LegacyResultCallback callback);

  void Result(const mojom::Result result,
              const std::string& queue_id,
              const std::string& contribution_id);

  void OnResult(mojom::ContributionInfoPtr contribution,
                const mojom::Result result,
                const std::string& queue_id);

  void SetRetryTimer(const std::string& contribution_id, base::TimeDelta delay);

  void OnRetryTimerElapsed(const std::string& contribution_id);

  void SetRetryCounter(mojom::ContributionInfoPtr contribution);

  void Retry(const mojom::Result result,
             std::shared_ptr<mojom::ContributionInfoPtr> shared_contribution);

  void OnMarkUnblindedTokensAsSpendable(const mojom::Result result,
                                        const std::string& contribution_id);

  const raw_ref<RewardsEngineImpl> engine_;
  Unblinded unblinded_;
  ContributionSKU sku_;
  ContributionMonthly monthly_;
  ContributionAC ac_;
  ContributionTip tip_;
  ContributionExternalWallet external_wallet_;
  std::map<std::string, ContributionRequest> requests_;
  base::OneShotTimer auto_contribute_timer_;
  base::OneShotTimer monthly_contribution_timer_;
  std::map<std::string, base::OneShotTimer> retry_timers_;
  base::OneShotTimer queue_timer_;
  bool queue_in_progress_ = false;
  bool monthly_contributions_processing_ = false;
};

}  // namespace contribution
}  // namespace brave_rewards::internal
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_H_
