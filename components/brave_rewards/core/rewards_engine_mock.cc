/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/rewards_engine_mock.h"

#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"

using ::testing::Return;

namespace brave_rewards::internal {

AddMockRewardsClient::AddMockRewardsClient() = default;

AddMockRewardsClient::~AddMockRewardsClient() = default;

MockRewardsEngine::MockRewardsEngine()
    : RewardsEngine(
          mock_client_receiver_.BindNewEndpointAndPassDedicatedRemote(),
          mojom::RewardsEngineOptions()) {
  Get<EnvironmentConfig>().AllowDefaultValuesForTesting();
  ON_CALL(*this, database()).WillByDefault(Return(&mock_database_));
}

MockRewardsEngine::~MockRewardsEngine() = default;

MockRewardsEngineClient* MockRewardsEngine::mock_client() {
  return &mock_client_;
}

database::MockDatabase* MockRewardsEngine::mock_database() {
  return &mock_database_;
}

}  // namespace brave_rewards::internal
