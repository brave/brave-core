/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::confirmations {

namespace {

void FailedToMigrate(InitializeCallback callback) {
  std::move(callback).Run(/*success*/ false);
}

void SuccessfullyMigrated(InitializeCallback callback) {
  AdsClientHelper::GetInstance()->SetBooleanPref(
      prefs::kHasMigratedConfirmationState, true);
  std::move(callback).Run(/*success*/ true);
}

}  // namespace

void Migrate(InitializeCallback callback) {
  if (HasMigrated()) {
    return std::move(callback).Run(/*success*/ true);
  }

  AdsClientHelper::GetInstance()->Load(
      kConfirmationStateFilename,
      base::BindOnce(
          [](InitializeCallback callback,
             const absl::optional<std::string>& json) {
            if (!json) {
              // Confirmation state does not exist
              return SuccessfullyMigrated(std::move(callback));
            }

            if (!ConfirmationStateManager::GetInstance().FromJson(*json)) {
              BLOG(0, "Failed to load confirmation state");
              return FailedToMigrate(std::move(callback));
            }

            BLOG(1, "Migrating confirmation state");

            const std::string migrated_json =
                ConfirmationStateManager::GetInstance().ToJson();
            SetHashForJson(migrated_json);

            AdsClientHelper::GetInstance()->Save(
                kConfirmationStateFilename, migrated_json,
                base::BindOnce(
                    [](InitializeCallback callback, const bool success) {
                      if (!success) {
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

}  // namespace brave_ads::confirmations
