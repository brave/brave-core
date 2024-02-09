/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_AC_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_AC_H_

#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace contribution {

class ContributionAC {
 public:
  explicit ContributionAC(RewardsEngineImpl& engine);

  ~ContributionAC();

  void Process(const uint64_t reconcile_stamp);

 private:
  void PreparePublisherList(std::vector<mojom::PublisherInfoPtr> list);

  void QueueSaved(const mojom::Result result);

  const raw_ref<RewardsEngineImpl> engine_;
  base::WeakPtrFactory<ContributionAC> weak_factory_{this};
};

}  // namespace contribution
}  // namespace brave_rewards::internal
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_AC_H_
