/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V14_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V14_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace state {

// Migration 14 corrects the situation where the `external_wallet_type` pref is
// empty but the user has a connected wallet. Users that connected before
// `external_wallet_type` was introduced may be in this state.
class StateMigrationV14 {
 public:
  explicit StateMigrationV14(RewardsEngine& engine);
  ~StateMigrationV14();

  void Migrate(ResultCallback);

 private:
  bool MigrateExternalWallet(const std::string& wallet_type);

  const raw_ref<RewardsEngine> engine_;
};

}  // namespace state
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V14_H_
