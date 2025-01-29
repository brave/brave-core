/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_STATE_STATE_MIGRATION_V12_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_STATE_STATE_MIGRATION_V12_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/engine/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace state {

class StateMigrationV12 {
 public:
  explicit StateMigrationV12(RewardsEngine& engine);
  ~StateMigrationV12();

  void Migrate(ResultCallback);

 private:
  bool MigrateExternalWallet(const std::string& wallet_type);

  const raw_ref<RewardsEngine> engine_;
};

}  // namespace state
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_STATE_STATE_MIGRATION_V12_H_
