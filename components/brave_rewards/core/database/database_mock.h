/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_MOCK_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_rewards::internal {
namespace database {

class MockDatabase : public Database {
 public:
  explicit MockDatabase(RewardsEngineImpl& engine);

  ~MockDatabase() override;

  MOCK_METHOD2(GetContributionInfo,
               void(const std::string& contribution_id,
                    GetContributionInfoCallback callback));

  MOCK_METHOD2(GetSpendableUnblindedTokensByBatchTypes,
               void(const std::vector<mojom::CredsBatchType>& batch_types,
                    GetUnblindedTokenListCallback callback));
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_MOCK_H_
