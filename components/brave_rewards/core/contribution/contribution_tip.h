/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_TIP_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_TIP_H_

#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {

class RewardsEngineImpl;

namespace contribution {

class ContributionTip {
 public:
  explicit ContributionTip(RewardsEngineImpl& engine);

  ~ContributionTip();

  using ProcessCallback = base::OnceCallback<void(std::optional<std::string>)>;

  void Process(const std::string& publisher_id,
               double amount,
               ProcessCallback callback);

 private:
  void OnPublisherDataRead(const std::string& publisher_id,
                           double amount,
                           ProcessCallback callback,
                           mojom::ServerPublisherInfoPtr server_info);

  void OnQueueSaved(const std::string& queue_id,
                    ProcessCallback callback,
                    mojom::Result result);

  void OnPendingTipSaved(mojom::Result result);

  const raw_ref<RewardsEngineImpl> engine_;
  base::WeakPtrFactory<ContributionTip> weak_factory_{this};
};

}  // namespace contribution
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_TIP_H_
