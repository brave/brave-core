/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/confirmations/legacy_confirmation_migration.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "bat/ads/internal/legacy_migration/confirmations/legacy_confirmation_migration_util.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads::confirmations {

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
    std::move(callback).Run(/*success*/ true);
    return;
  }

  BLOG(3, "Loading confirmation state");

  AdsClientHelper::GetInstance()->Load(
      kConfirmationStateFilename,
      base::BindOnce(
          [](InitializeCallback callback, const bool success,
             const std::string& json) {
            if (!success) {
              // Confirmation state does not exist
              SuccessfullyMigrated(std::move(callback));
              return;
            }

            if (!ConfirmationStateManager::GetInstance()->FromJson(json)) {
              BLOG(0, "Failed to load confirmation state");
              FailedToMigrate(std::move(callback));
              return;
            }

            BLOG(3, "Successfully loaded confirmation state");

            BLOG(1, "Migrating confirmation state");

            const std::string migrated_json =
                ConfirmationStateManager::GetInstance()->ToJson();
            SetHashForJson(migrated_json);

            AdsClientHelper::GetInstance()->Save(
                kConfirmationStateFilename, migrated_json,
                base::BindOnce(
                    [](InitializeCallback callback, const bool success) {
                      if (!success) {
                        BLOG(0, "Failed to save confirmation state");
                        FailedToMigrate(std::move(callback));
                        return;
                      }

                      BLOG(3, "Successfully migrated confirmation state");
                      SuccessfullyMigrated(std::move(callback));
                    },
                    std::move(callback)));
          },
          std::move(callback)));
}

}  // namespace ads::confirmations
