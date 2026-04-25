/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/initialization_manager.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_rewards/core/engine/contribution/contribution.h"
#include "brave/components/brave_rewards/core/engine/database/database.h"
#include "brave/components/brave_rewards/core/engine/migrations/pref_migration_manager.h"
#include "brave/components/brave_rewards/core/engine/parameters/rewards_parameters_provider.h"
#include "brave/components/brave_rewards/core/engine/publisher/publisher.h"
#include "brave/components/brave_rewards/core/engine/uphold/uphold.h"
#include "brave/components/brave_rewards/core/engine/wallet_provider/linkage_checker.h"

namespace brave_rewards::internal {

InitializationManager::InitializationManager(RewardsEngine& engine)
    : RewardsEngineHelper(engine) {}

InitializationManager::~InitializationManager() = default;

void InitializationManager::Initialize(InitializeCallback callback) {
  if (state_ != State::kUninitialized) {
    LogError(FROM_HERE) << "Initialization has already been started";
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), false));
    return;
  }

  state_ = State::kInitializing;

  engine().database()->Initialize(
      base::BindOnce(&InitializationManager::OnDatabaseInitialized,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void InitializationManager::Shutdown(ShutdownCallback callback) {
  if (state_ != State::kReady) {
    LogError(FROM_HERE) << "Initialization not complete";
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), false));
    return;
  }

  state_ = State::kShuttingDown;

  client().ClearAllNotifications();

  engine().database()->FinishAllInProgressContributions(
      base::BindOnce(&InitializationManager::OnContributionsFinished,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void InitializationManager::OnDatabaseInitialized(InitializeCallback callback,
                                                  mojom::Result result) {
  DCHECK(state_ == State::kInitializing);

  if (result != mojom::Result::OK) {
    LogError(FROM_HERE) << "Database could not be initialized";
    std::move(callback).Run(false);
    return;
  }

  Get<PrefMigrationManager>().MigratePrefs(
      base::BindOnce(&InitializationManager::OnPrefsMigrated,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void InitializationManager::OnPrefsMigrated(InitializeCallback callback) {
  DCHECK(state_ == State::kInitializing);

  InitializeHelpers();

  state_ = State::kReady;

  std::move(callback).Run(true);
}

void InitializationManager::InitializeHelpers() {
  engine().publisher()->SetPublisherServerListTimer();
  engine().contribution()->SetReconcileStampTimer();
  engine().contribution()->SetMonthlyContributionTimer();
  engine().contribution()->Initialize();
  engine().Get<RewardsParametersProvider>().StartAutoUpdate();
  engine().uphold()->CheckEligibility();
  engine().Get<LinkageChecker>().Start();
}

void InitializationManager::OnContributionsFinished(ShutdownCallback callback,
                                                    mojom::Result result) {
  if (result != mojom::Result::OK) {
    LogError(FROM_HERE) << "Error finalizing contributions";
  }

  engine().database()->Close(
      base::BindOnce(&InitializationManager::OnDatabaseClosed,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void InitializationManager::OnDatabaseClosed(ShutdownCallback callback,
                                             mojom::Result result) {
  if (result != mojom::Result::OK) {
    LogError(FROM_HERE) << "Error closing database";
  }

  state_ = State::kUninitialized;
  std::move(callback).Run(true);
}

}  // namespace brave_rewards::internal
