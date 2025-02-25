/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/state/state_migration_v15.h"

#include <utility>

#include "brave/components/brave_rewards/core/engine/rewards_engine.h"
#include "brave/components/brave_rewards/core/engine/state/state_keys.h"

namespace brave_rewards::internal::state {

StateMigrationV15::StateMigrationV15(RewardsEngine& engine) : engine_(engine) {}

StateMigrationV15::~StateMigrationV15() = default;

void StateMigrationV15::Migrate(ResultCallback callback) {
  engine_->SetState<uint64_t>(kServerPublisherListStamp, 0);
  std::move(callback).Run(mojom::Result::OK);
}

}  // namespace brave_rewards::internal::state
