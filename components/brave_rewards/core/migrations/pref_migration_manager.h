/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_MIGRATIONS_PREF_MIGRATION_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_MIGRATIONS_PREF_MIGRATION_MANAGER_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"

namespace brave_rewards::internal {

class Prefs;

// Responsible for performing migrations on data stored in user preferences.
class PrefMigrationManager : public RewardsEngineHelper,
                             public WithHelperKey<PrefMigrationManager> {
 public:
  explicit PrefMigrationManager(RewardsEngine& engine);
  ~PrefMigrationManager() override;

  // Migrates the user to the current pref version.
  void MigratePrefs(base::OnceClosure callback);

  void MigratePrefsForTesting(int target_version, base::OnceClosure callback);

  static int GetCurrentVersionForTesting();

 private:
  Prefs& user_prefs();

  void MigratePrefsToVersion(int target_version, base::OnceClosure callback);

  template <int... kVersions>
  void PerformMigrations(int target_version,
                         std::integer_sequence<int, kVersions...>);

  template <int kVersion>
  void MaybePerformMigration(int target_version);

  template <int kVersion>
  void MigrateToVersion();

  base::WeakPtrFactory<PrefMigrationManager> weak_factory_{this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_MIGRATIONS_PREF_MIGRATION_MANAGER_H_
