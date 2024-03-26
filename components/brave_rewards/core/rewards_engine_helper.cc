/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/rewards_engine_helper.h"

namespace brave_rewards::internal {

RewardsEngineHelper::~RewardsEngineHelper() = default;

RewardsEngineHelper::RewardsEngineHelper(RewardsEngine& engine)
    : engine_(engine) {}

mojom::RewardsEngineClient& RewardsEngineHelper::client() {
  auto* client = engine().client();
  CHECK(client);
  return *client;
}

RewardsLogStream RewardsEngineHelper::Log(base::Location location) {
  return engine_->Log(location);
}

RewardsLogStream RewardsEngineHelper::LogError(base::Location location) {
  return engine_->LogError(location);
}

}  // namespace brave_rewards::internal
