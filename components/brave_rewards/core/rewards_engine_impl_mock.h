/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_IMPL_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_IMPL_MOCK_H_

#include "brave/components/brave_rewards/core/database/database_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_rewards::internal {

class AddMockRewardsClient {
 protected:
  AddMockRewardsClient();

  ~AddMockRewardsClient();

  MockRewardsEngineClient mock_client_;
  mojo::AssociatedReceiver<mojom::RewardsEngineClient> mock_client_receiver_{
      &mock_client_};
};

class MockRewardsEngineImpl : private AddMockRewardsClient,
                              public RewardsEngineImpl {
 public:
  MockRewardsEngineImpl();

  ~MockRewardsEngineImpl() override;

  MockRewardsEngineClient* mock_client();

  database::MockDatabase* mock_database();

  MOCK_METHOD0(database, database::Database*());

 private:
  database::MockDatabase mock_database_{*this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_IMPL_MOCK_H_
