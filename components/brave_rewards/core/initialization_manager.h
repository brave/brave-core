/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_INITIALIZATION_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_INITIALIZATION_MANAGER_H_

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"

namespace brave_rewards::internal {

// Responsible for performing all necessary Rewards engine initialization,
// including database migrations, preference migrations, and startup of
// background tasks.
class InitializationManager : public RewardsEngineHelper,
                              public WithHelperKey<InitializationManager> {
 public:
  enum class State { kUninitialized, kInitializing, kReady, kShuttingDown };

  explicit InitializationManager(RewardsEngine& engine);
  ~InitializationManager() override;

  using InitializeCallback = base::OnceCallback<void(bool)>;
  void Initialize(InitializeCallback callback);

  using ShutdownCallback = base::OnceCallback<void(bool)>;
  void Shutdown(ShutdownCallback callback);

  State state() const { return state_; }

  bool is_ready() const { return state_ == State::kReady; }
  bool is_shutting_down() const { return state_ == State::kShuttingDown; }

 private:
  void OnDatabaseInitialized(InitializeCallback callback, mojom::Result result);
  void OnPrefsMigrated(InitializeCallback callback);
  void InitializeHelpers();
  void OnContributionsFinished(ShutdownCallback callback, mojom::Result result);
  void OnDatabaseClosed(ShutdownCallback callback, mojom::Result result);

  State state_ = State::kUninitialized;
  base::WeakPtrFactory<InitializationManager> weak_factory_{this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_INITIALIZATION_MANAGER_H_
