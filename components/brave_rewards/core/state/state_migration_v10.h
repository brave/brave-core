/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V10_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V10_H_

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/endpoints/brave/get_wallet.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace state {

class StateMigrationV10 {
 public:
  explicit StateMigrationV10(RewardsEngineImpl& engine);
  ~StateMigrationV10();

  void Migrate(ResultCallback callback);

 private:
  void OnGetWallet(ResultCallback callback,
                   endpoints::GetWallet::Result&& result);

  const raw_ref<RewardsEngineImpl> engine_;
  base::WeakPtrFactory<StateMigrationV10> weak_factory_{this};
};

}  // namespace state
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V10_H_
