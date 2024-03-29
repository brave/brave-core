/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_MIGRATIONS_DATABASE_MIGRATION_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_MIGRATIONS_DATABASE_MIGRATION_MANAGER_H_

#include <utility>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/common/sql_store.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"

namespace brave_rewards::internal {

// Responsible for performing migrations on the Rewards database.
class DatabaseMigrationManager
    : public RewardsEngineHelper,
      public WithHelperKey<DatabaseMigrationManager> {
 public:
  explicit DatabaseMigrationManager(RewardsEngine& engine);
  ~DatabaseMigrationManager() override;

  using MigrateCallback = base::OnceCallback<void(bool)>;

  // Migrates the database to the current version.
  void MigrateDatabase(base::OnceCallback<void(bool)> callback);

  void MigrateDatabaseForTesting(int target_version, MigrateCallback callback);

  static int GetCurrentVersionForTesting();

 private:
  void MigrateDatabaseToVersion(int target_version, MigrateCallback callback);

  void OnInitialized(int target_version,
                     MigrateCallback callback,
                     SQLReader reader);

  template <int kVersion>
  mojom::DBCommandPtr Migration();

  template <int... kVersions>
  SQLStore::CommandList GetMigrationCommands(
      int db_version,
      int target_version,
      std::integer_sequence<int, kVersions...>);

  template <int kVersion>
  void MaybeAddMigration(SQLStore::CommandList& commands,
                         int db_version,
                         int target_version);

  void OnMigrationComplete(MigrateCallback callback, SQLReader reader);

  base::WeakPtrFactory<DatabaseMigrationManager> weak_factory_{this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_MIGRATIONS_DATABASE_MIGRATION_MANAGER_H_
