/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_HELPER_H_

#include "base/memory/raw_ref.h"
#include "base/supports_user_data.h"
#include "brave/components/brave_rewards/common/mojom/rewards_engine.mojom.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/rewards_log_stream.h"

namespace brave_rewards::internal {

// Base class for Rewards engine helpers. Provides convenient accessors and
// utility methods.
class RewardsEngineHelper : public base::SupportsUserData::Data {
 public:
  ~RewardsEngineHelper() override;

 protected:
  explicit RewardsEngineHelper(RewardsEngine& engine);

  RewardsEngineHelper(const RewardsEngineHelper&) = delete;
  RewardsEngineHelper& operator=(const RewardsEngineHelper&) = delete;

  RewardsEngine& engine() { return engine_.get(); }
  const RewardsEngine& engine() const { return engine_.get(); }

  mojom::RewardsEngineClient& client();

  RewardsLogStream Log(base::Location location);
  RewardsLogStream LogError(base::Location location);

  template <typename T>
  T& Get() const {
    return engine_->Get<T>();
  }

 private:
  const raw_ref<RewardsEngine> engine_;
};

// A mixin for exposing a user data key for a `RewardsEngineHelper` class.
template <typename T>
class WithHelperKey {
 public:
  static const void* GetHelperKey() { return &kHelperKey; }

 private:
  static constexpr int kHelperKey = 0;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_HELPER_H_
