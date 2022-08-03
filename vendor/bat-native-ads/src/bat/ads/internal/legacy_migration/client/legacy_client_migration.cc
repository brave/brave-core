/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/client/legacy_client_migration.h"

#include <string>

#include "base/bind.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/client/client_info.h"
#include "bat/ads/internal/deprecated/client/client_state_manager_constants.h"
#include "bat/ads/internal/legacy_migration/client/legacy_client_migration_util.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace client {

namespace {

void FailedToMigrate(InitializeCallback callback) {
  callback(/* success */ false);
}

void SuccessfullyMigrated(InitializeCallback callback) {
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kHasMigratedClientState,
                                                 true);
  callback(/* success */ true);
}

}  // namespace

void Migrate(InitializeCallback callback) {
  if (HasMigrated()) {
    callback(/* success */ true);
    return;
  }

  BLOG(3, "Loading client state");

  AdsClientHelper::GetInstance()->Load(
      kClientStateFilename,
      base::BindOnce(
          [](InitializeCallback callback, const bool success,
             const std::string& json) {
            if (!success) {
              // Client state does not exist
              SuccessfullyMigrated(callback);
              return;
            }

            ClientInfo client;
            if (!client.FromJson(json)) {
              BLOG(0, "Failed to load client state");
              FailedToMigrate(callback);
              return;
            }

            BLOG(3, "Successfully loaded client state");

            BLOG(1, "Migrating client state");

            const std::string migrated_json = client.ToJson();
            SetHashForJson(migrated_json);

            AdsClientHelper::GetInstance()->Save(
                kClientStateFilename, migrated_json,
                base::BindOnce(
                    [](InitializeCallback callback, const bool success) {
                      if (!success) {
                        BLOG(0, "Failed to save client state");
                        FailedToMigrate(callback);
                        return;
                      }

                      BLOG(3, "Successfully migrated client state");
                      SuccessfullyMigrated(callback);
                    },
                    callback));
          },
          callback));
}

}  // namespace client
}  // namespace ads
