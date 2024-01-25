/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_

#include <optional>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace contribution {

class ContributionMonthly {
 public:
  explicit ContributionMonthly(RewardsEngineImpl& engine);

  ~ContributionMonthly();

  void Process(std::optional<base::Time> cutoff_time, ResultCallback callback);

 private:
  void AdvanceContributionDates(
      std::optional<base::Time> cutoff_time,
      ResultCallback callback,
      std::vector<mojom::PublisherInfoPtr> publishers);

  void OnNextContributionDateAdvanced(
      std::vector<mojom::PublisherInfoPtr> publishers,
      ResultCallback callback,
      bool success);

  const raw_ref<RewardsEngineImpl> engine_;
  base::WeakPtrFactory<ContributionMonthly> weak_factory_{this};
};

}  // namespace contribution
}  // namespace brave_rewards::internal
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_
