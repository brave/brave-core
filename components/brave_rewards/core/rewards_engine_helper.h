/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_HELPER_H_

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/common/mojom/rewards_engine.mojom.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/rewards_log_stream.h"

namespace brave_rewards::internal {

// Base class for Rewards engine helpers. Provides convenient accessors and
// utility methods.
class RewardsEngineHelper {
 protected:
  explicit RewardsEngineHelper(RewardsEngineImpl& engine);
  virtual ~RewardsEngineHelper();

  RewardsEngineHelper(const RewardsEngineHelper&) = delete;
  RewardsEngineHelper& operator=(const RewardsEngineHelper&) = delete;

  RewardsEngineImpl& engine() { return engine_.get(); }
  const RewardsEngineImpl& engine() const { return engine_.get(); }

  mojom::RewardsEngineClient& client();

  RewardsLogStream Log(base::Location location);
  RewardsLogStream LogError(base::Location location);

  template <typename T>
  T& Get() const {
    return engine_->Get<T>();
  }

 private:
  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_HELPER_H_
