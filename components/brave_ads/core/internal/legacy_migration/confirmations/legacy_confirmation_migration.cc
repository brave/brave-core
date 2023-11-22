/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration.h"

#include <string>
#include <utility>

#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

void FailedToMigrate(InitializeCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void SuccessfullyMigrated(InitializeCallback callback) {
  SetProfileBooleanPref(prefs::kHasMigratedConfirmationState, true);
  std::move(callback).Run(/*success=*/true);
}

}  // namespace

void MigrateConfirmationState(InitializeCallback callback) {
  if (HasMigratedConfirmation()) {
    return std::move(callback).Run(/*success=*/true);
  }

  Load(kConfirmationStateFilename,
       base::BindOnce(
           [](InitializeCallback callback,
              const absl::optional<std::string>& json) {
             if (!json) {
               // Confirmation state does not exist
               return SuccessfullyMigrated(std::move(callback));
             }

             if (!ConfirmationStateManager::GetInstance().FromJson(*json)) {
               // TODO(https://github.com/brave/brave-browser/issues/32066):
               // Remove migration failure dumps.
               base::debug::DumpWithoutCrashing();

               BLOG(0, "Failed to load confirmation state");
               return FailedToMigrate(std::move(callback));
             }

             BLOG(1, "Migrating confirmation state");

             const std::string migrated_json =
                 ConfirmationStateManager::GetInstance().ToJson();

             Save(kConfirmationStateFilename, migrated_json,
                  base::BindOnce(
                      [](InitializeCallback callback, const bool success) {
                        if (!success) {
                          // TODO(https://github.com/brave/brave-browser/issues/32066):
                          // Remove migration failure dumps.
                          base::debug::DumpWithoutCrashing();

                          BLOG(0, "Failed to save confirmation state");
                          return FailedToMigrate(std::move(callback));
                        }

                        BLOG(3, "Successfully migrated confirmation state");
                        SuccessfullyMigrated(std::move(callback));
                      },
                      std::move(callback)));
           },
           std::move(callback)));
}

}  // namespace brave_ads
