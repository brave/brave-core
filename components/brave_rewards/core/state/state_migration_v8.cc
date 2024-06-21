/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/state/state_migration_v8.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"

namespace brave_rewards::internal::state {

StateMigrationV8::StateMigrationV8(RewardsEngine& engine) : engine_(engine) {}

StateMigrationV8::~StateMigrationV8() = default;

void StateMigrationV8::Migrate(ResultCallback callback) {
  const bool enabled = engine_->GetState<bool>("enabled");

  if (!enabled) {
    engine_->SetState(kAutoContributeEnabled, false);
  }

  std::move(callback).Run(mojom::Result::OK);
}

}  // namespace brave_rewards::internal::state
