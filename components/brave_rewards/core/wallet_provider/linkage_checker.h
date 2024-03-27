/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_LINKAGE_CHECKER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_LINKAGE_CHECKER_H_

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/brave_rewards/core/endpoints/brave/get_wallet.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"

namespace brave_rewards::internal {

// Responsible for checking the Rewards external wallet linkage status
// periodically, or upon request. If the Rewards account was previously linked
// and then unlinked on the server, then it will transition the user back into
// an unlinked state. It will also check for the completion of any polling-based
// linking flow.
class LinkageChecker : public RewardsEngineHelper,
                       public WithHelperKey<LinkageChecker> {
 public:
  explicit LinkageChecker(RewardsEngine& engine);
  ~LinkageChecker() override;

  // Starts the wallet linkage checker, if not already started. If not already
  // started, the check will be run immediately and then on a timer.
  void Start();

  // Stops the wallet linkage checker.
  void Stop();

  // Checks wallet linkage status immediately.
  void CheckLinkage();

 private:
  bool ShouldPerformCheck();
  mojom::ExternalWalletPtr GetExternalWallet();
  void MaybeUpdateExternalWalletStatus(endpoints::GetWallet::Value& value);
  void UpdateSelfCustodyAvailableDict(endpoints::GetWallet::Value& value);
  void CheckLinkageCallback(endpoints::GetWallet::Result&& result);

  bool check_in_progress_ = false;
  base::RepeatingTimer timer_;
  base::WeakPtrFactory<LinkageChecker> weak_factory_{this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_LINKAGE_CHECKER_H_
