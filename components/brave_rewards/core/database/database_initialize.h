/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_INITIALIZE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_INITIALIZE_H_

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/database/database_migration.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace database {

class DatabaseInitialize {
 public:
  explicit DatabaseInitialize(RewardsEngineImpl& engine);
  ~DatabaseInitialize();

  void Start(ResultCallback callback);

 private:
  void OnInitialize(ResultCallback callback,
                    mojom::DBCommandResponsePtr response);

  const raw_ref<RewardsEngineImpl> engine_;
  DatabaseMigration migration_;
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_INITIALIZE_H_
