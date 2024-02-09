/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_

#include <map>
#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace contribution {

class ContributionExternalWallet {
 public:
  explicit ContributionExternalWallet(RewardsEngineImpl& engine);

  ~ContributionExternalWallet();

  void Process(const std::string& contribution_id, ResultCallback callback);

  void Retry(mojom::ContributionInfoPtr contribution, ResultCallback callback);

 private:
  void ContributionInfo(ResultCallback callback,
                        mojom::ContributionInfoPtr contribution);

  void OnServerPublisherInfo(const std::string& contribution_id,
                             double amount,
                             mojom::RewardsType type,
                             mojom::ContributionProcessor processor,
                             bool single_publisher,
                             ResultCallback callback,
                             mojom::ServerPublisherInfoPtr info);

  void Completed(bool single_publisher,
                 ResultCallback callback,
                 mojom::Result result);

  const raw_ref<RewardsEngineImpl> engine_;
  base::WeakPtrFactory<ContributionExternalWallet> weak_factory_{this};
};

}  // namespace contribution
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_EXTERNAL_WALLET_H_
