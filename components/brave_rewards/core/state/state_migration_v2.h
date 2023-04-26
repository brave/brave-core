/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V2_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V2_H_

#include <memory>
#include <string>

#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/legacy/bat_state.h"

namespace brave_rewards::internal::state {

class StateMigrationV2 {
 public:
  StateMigrationV2();
  ~StateMigrationV2();

  void Migrate(LegacyResultCallback callback);

 private:
  void OnLoadState(mojom::Result result, LegacyResultCallback callback);

  std::unique_ptr<LegacyBatState> legacy_state_;
};

}  // namespace brave_rewards::internal::state

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V2_H_
