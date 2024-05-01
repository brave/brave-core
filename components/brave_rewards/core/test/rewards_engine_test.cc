/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

#include "brave/components/brave_rewards/core/common/environment_config.h"

namespace brave_rewards::internal {

RewardsEngineTest::RewardsEngineTest()
    : RewardsEngineTest(std::make_unique<TestRewardsEngineClient>()) {}

RewardsEngineTest::RewardsEngineTest(
    std::unique_ptr<TestRewardsEngineClient> client)
    : client_(std::move(client)),
      client_receiver_(client_.get()),
      engine_(client_receiver_.BindNewEndpointAndPassDedicatedRemote(),
              mojom::RewardsEngineOptions()) {
  engine().Get<EnvironmentConfig>().AllowDefaultValuesForTesting();
}

RewardsEngineTest::~RewardsEngineTest() = default;

void RewardsEngineTest::InitializeEngine() {
  auto result = WaitFor<mojom::Result>(
      [&](auto callback) { engine().Initialize(std::move(callback)); });
  DCHECK(result == mojom::Result::OK);
}

}  // namespace brave_rewards::internal
