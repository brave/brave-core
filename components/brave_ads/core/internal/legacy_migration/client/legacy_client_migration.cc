/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration.h"

#include <optional>
#include <string>
#include <utility>

#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_info.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_database_table.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_ad_history_json_reader.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

namespace {

void FailedToMigrate(const std::string& reason, InitializeCallback callback) {
  SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason", reason);
  base::debug::DumpWithoutCrashing();
  BLOG(0, reason);
  std::move(callback).Run(/*success=*/false);
}

void SuccessfullyMigrated(InitializeCallback callback) {
  SetProfileBooleanPref(prefs::kHasMigratedClientState, true);
  std::move(callback).Run(/*success=*/true);
}

void HandleAdHistoryMigration(const std::string& json,
                              InitializeCallback callback) {
  const std::optional<AdHistoryList> ad_history =
      json::reader::ReadAdHistory(json);
  if (!ad_history) {
    BLOG(3, "Successfully migrated client state");
    return SuccessfullyMigrated(std::move(callback));
  }

  database::table::AdHistory database_table;
  database_table.Save(*ad_history,
                      base::BindOnce(
                          [](InitializeCallback callback, const bool success) {
                            if (!success) {
                              return FailedToMigrate(
                                  "Failed to migrate ad history client state",
                                  std::move(callback));
                            }

                            BLOG(3, "Successfully migrated client state");
                            SuccessfullyMigrated(std::move(callback));
                          },
                          std::move(callback)));
}

void HandleMalformedClientState(InitializeCallback callback) {
  BLOG(0, "Resetting malformed client state to default values");

  GetAdsClient().Save(
      kClientJsonFilename, /*default state=§*/ "{}",
      base::BindOnce(
          [](InitializeCallback callback, const bool success) {
            if (!success) {
              return FailedToMigrate(
                  "Failed to reset malformed client state to default values",
                  std::move(callback));
            }

            SuccessfullyMigrated(std::move(callback));
          },
          std::move(callback)));
}

void HandleClientStateMigration(InitializeCallback callback,
                                const std::optional<std::string>& json) {
  if (!json) {
    // No client state to migrate.
    return SuccessfullyMigrated(std::move(callback));
  }

  BLOG(1, "Migrating client state");

  ClientInfo client;
  if (!client.FromJson(*json)) {
    return HandleMalformedClientState(std::move(callback));
  }

  const std::string migrated_json = client.ToJson();
  GetAdsClient().Save(
      kClientJsonFilename, migrated_json,
      base::BindOnce(
          [](const std::string& json, InitializeCallback callback,
             const bool success) {
            if (!success) {
              return FailedToMigrate("Failed to save migrated client state",
                                     std::move(callback));
            }

            HandleAdHistoryMigration(json, std::move(callback));
          },
          *json, std::move(callback)));
}

}  // namespace

void MigrateClientState(InitializeCallback callback) {
  if (HasMigratedClientState()) {
    // Already migrated.
    return std::move(callback).Run(/*success=*/true);
  }

  GetAdsClient().Load(
      kClientJsonFilename,
      base::BindOnce(&HandleClientStateMigration, std::move(callback)));
}

}  // namespace brave_ads
