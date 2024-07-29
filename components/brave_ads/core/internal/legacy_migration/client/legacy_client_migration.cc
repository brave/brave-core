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
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_util.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

namespace {

void FailedToMigrate(InitializeCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void SuccessfullyMigrated(InitializeCallback callback) {
  SetProfileBooleanPref("brave.brave_ads.state.has_migrated.client.v6", true);
  SetProfileBooleanPref(prefs::kHasMigratedClientState, true);
  std::move(callback).Run(/*success=*/true);
}

}  // namespace

void MigrateClientState(InitializeCallback callback) {
  if (HasMigratedClientState()) {
    return std::move(callback).Run(/*success=*/true);
  }

  Load(kClientJsonFilename,
       base::BindOnce(
           [](InitializeCallback callback,
              const std::optional<std::string>& json) {
             if (!json) {
               // Client state does not exist.
               return SuccessfullyMigrated(std::move(callback));
             }
             std::string mutable_json = *json;

             ClientInfo client;

             if (!GetProfileBooleanPref(
                     "brave.brave_ads.state.has_migrated.client.v6") &&
                 !client.FromJson(mutable_json)) {
               // The client state is corrupted, therefore, reset it to the
               // default values for version 6.
               BLOG(0,
                    "Client state is corrupted, resetting to default values");
               mutable_json = "{}";
             }

             client = {};
             if (!client.FromJson(mutable_json)) {
               // TODO(https://github.com/brave/brave-browser/issues/32066):
               // Detect potential defects using `DumpWithoutCrashing`.
               SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                         "Failed to parse client state");
               base::debug::DumpWithoutCrashing();

               BLOG(0, "Failed to parse client state");

               return FailedToMigrate(std::move(callback));
             }

             BLOG(1, "Migrating client state");

             Save(kClientJsonFilename, client.ToJson(),
                  base::BindOnce(
                      [](InitializeCallback callback, const bool success) {
                        if (!success) {
                          // TODO(https://github.com/brave/brave-browser/issues/32066):
                          // Detect potential defects using
                          // `DumpWithoutCrashing`.
                          SCOPED_CRASH_KEY_STRING64(
                              "Issue32066", "failure_reason",
                              "Failed to migrate client state");
                          base::debug::DumpWithoutCrashing();

                          BLOG(0, "Failed to migrate client state");

                          return FailedToMigrate(std::move(callback));
                        }

                        BLOG(3, "Successfully migrated client state");
                        SuccessfullyMigrated(std::move(callback));
                      },
                      std::move(callback)));
           },
           std::move(callback)));
}

}  // namespace brave_ads
