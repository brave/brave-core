/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V1_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V1_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/legacy/publisher_state.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace state {

class StateMigrationV1 {
 public:
  explicit StateMigrationV1(RewardsEngineImpl& engine);
  ~StateMigrationV1();

  void Migrate(ResultCallback callback);

  bool legacy_data_migrated() const { return legacy_data_migrated_; }

 private:
  void OnLoadState(ResultCallback callback, mojom::Result result);

  void BalanceReportsSaved(ResultCallback callback, mojom::Result result);

  std::unique_ptr<publisher::LegacyPublisherState> legacy_publisher_;
  const raw_ref<RewardsEngineImpl> engine_;
  bool legacy_data_migrated_ = false;
  base::WeakPtrFactory<StateMigrationV1> weak_factory_{this};
};

}  // namespace state
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_MIGRATION_V1_H_
