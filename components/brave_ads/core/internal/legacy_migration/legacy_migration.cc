/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/legacy_migration.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/legacy_migration/legacy_migration_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

namespace {

void SuccessfullyMigrated(InitializeCallback callback) {
  SetProfileBooleanPref(prefs::kHasMigratedState, true);
  std::move(callback).Run(/*success=*/true);
}

}  // namespace

void MigrateState(InitializeCallback callback) {
  if (HasMigratedState()) {
    // Already migrated.
    return std::move(callback).Run(/*success=*/true);
  }

  SuccessfullyMigrated(std::move(callback));
}

}  // namespace brave_ads
