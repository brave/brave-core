/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"

#include <utility>

using ::testing::Return;

namespace brave_rewards::internal {

AddMockRewardsClient::AddMockRewardsClient() = default;

AddMockRewardsClient::~AddMockRewardsClient() = default;

MockRewardsEngineImpl::MockRewardsEngineImpl()
    : RewardsEngineImpl(
          mock_client_receiver_.BindNewEndpointAndPassDedicatedRemote(),
          mojom::RewardsEngineOptions()) {
  ON_CALL(*this, database()).WillByDefault(Return(&mock_database_));
}

MockRewardsEngineImpl::~MockRewardsEngineImpl() = default;

MockRewardsEngineClient* MockRewardsEngineImpl::mock_client() {
  return &mock_client_;
}

database::MockDatabase* MockRewardsEngineImpl::mock_database() {
  return &mock_database_;
}

}  // namespace brave_rewards::internal
