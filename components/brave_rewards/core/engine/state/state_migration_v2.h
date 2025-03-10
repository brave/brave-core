/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_STATE_STATE_MIGRATION_V2_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_STATE_STATE_MIGRATION_V2_H_

#include <memory>
#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/engine/legacy/bat_state.h"
#include "brave/components/brave_rewards/core/engine/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace state {

class StateMigrationV2 {
 public:
  explicit StateMigrationV2(RewardsEngine& engine);
  ~StateMigrationV2();

  void Migrate(ResultCallback callback);

 private:
  void OnLoadState(ResultCallback callback, mojom::Result result);

  std::unique_ptr<LegacyBatState> legacy_state_;
  const raw_ref<RewardsEngine> engine_;
  base::WeakPtrFactory<StateMigrationV2> weak_factory_{this};
};

}  // namespace state
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_STATE_STATE_MIGRATION_V2_H_
