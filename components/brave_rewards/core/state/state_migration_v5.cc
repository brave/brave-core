/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/state/state_migration_v5.h"

#include <map>
#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"

namespace brave_rewards::internal::state {

StateMigrationV5::StateMigrationV5(RewardsEngine& engine) : engine_(engine) {}

StateMigrationV5::~StateMigrationV5() = default;

void StateMigrationV5::Migrate(ResultCallback callback) {
  const auto seed = engine_->GetState<std::string>(kRecoverySeed);
  if (seed.empty()) {
    std::move(callback).Run(mojom::Result::OK);
    return;
  }

  std::map<std::string, std::string> events;

  // Auto contribute
  auto enabled = engine_->GetState<bool>(kAutoContributeEnabled);
  events.insert(
      std::make_pair(kAutoContributeEnabled, std::to_string(enabled)));

  // Seed
  if (seed.size() > 1) {
    const std::string event_string = seed.substr(0, 2);
    events.insert(std::make_pair(kRecoverySeed, event_string));
  }

  // Payment id
  events.insert(
      std::make_pair(kPaymentId, engine_->GetState<std::string>(kPaymentId)));

  // Enabled
  enabled = engine_->GetState<bool>("enabled");
  events.insert(std::make_pair("enabled", std::to_string(enabled)));

  // Next reconcile
  const auto reconcile_stamp = engine_->GetState<uint64_t>(kNextReconcileStamp);
  events.insert(
      std::make_pair(kNextReconcileStamp, std::to_string(reconcile_stamp)));

  // Creation stamp
  const auto creation_stamp = engine_->GetState<uint64_t>(kCreationStamp);
  events.insert(std::make_pair(kCreationStamp, std::to_string(creation_stamp)));

  engine_->database()->SaveEventLogs(events, std::move(callback));
}

}  // namespace brave_rewards::internal::state
