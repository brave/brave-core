/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/confirmations/legacy_confirmation_migration.h"

#include <string>

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "bat/ads/internal/legacy_migration/confirmations/legacy_confirmation_migration_util.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace confirmations {

namespace {

void FailedToMigrate(InitializeCallback callback) {
  callback(/* success */ false);
}

void SuccessfullyMigrated(InitializeCallback callback) {
  AdsClientHelper::GetInstance()->SetBooleanPref(
      prefs::kHasMigratedConfirmationState, true);
  callback(/* success */ true);
}

}  // namespace

void Migrate(InitializeCallback callback) {
  if (HasMigrated()) {
    callback(/* success */ true);
    return;
  }

  BLOG(3, "Loading confirmation state");

  AdsClientHelper::GetInstance()->Load(
      kConfirmationStateFilename,
      [=](const bool success, const std::string& json) {
        if (!success) {
          // Confirmation state does not exist
          SuccessfullyMigrated(callback);
          return;
        }

        if (!ConfirmationStateManager::GetInstance()->FromJson(json)) {
          BLOG(0, "Failed to load confirmation state");
          FailedToMigrate(callback);
          return;
        }

        BLOG(3, "Successfully loaded confirmation state");

        BLOG(1, "Migrating confirmation state");

        const std::string migrated_json =
            ConfirmationStateManager::GetInstance()->ToJson();
        SetHashForJson(migrated_json);

        AdsClientHelper::GetInstance()->Save(
            kConfirmationStateFilename, migrated_json, [=](const bool success) {
              if (!success) {
                BLOG(0, "Failed to save confirmation state");
                FailedToMigrate(callback);
                return;
              }

              BLOG(3, "Successfully migrated confirmation state");
              SuccessfullyMigrated(callback);
            });
      });
}

}  // namespace confirmations
}  // namespace ads
