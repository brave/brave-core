/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration.h"

#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_confirmations_json_reader.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

namespace {

void FailedToMigrate(InitializeCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void SuccessfullyMigrated(InitializeCallback callback) {
  SetProfileBooleanPref("brave.brave_ads.state.has_migrated.confirmations.v8",
                        true);
  SetProfileBooleanPref(prefs::kHasMigratedConfirmationState, true);
  std::move(callback).Run(/*success=*/true);
}

}  // namespace

void MigrateConfirmationState(InitializeCallback callback) {
  if (HasMigratedConfirmation()) {
    return std::move(callback).Run(/*success=*/true);
  }

  GetAdsClient()->Load(
      kConfirmationsJsonFilename,
      base::BindOnce(
          [](InitializeCallback callback,
             const std::optional<std::string>& json) {
            if (!json) {
              // Confirmation state does not exist.
              return SuccessfullyMigrated(std::move(callback));
            }
            std::string mutable_json = *json;

            if (!GetProfileBooleanPref(
                    "brave.brave_ads.state.has_migrated.confirmations.v8") &&
                !ConfirmationStateManager::GetInstance().FromJson(
                    mutable_json)) {
              // The confirmation state is corrupted, therefore, reset it to
              // the default values for version 8.
              BLOG(0,
                   "Confirmation state is corrupted, resetting to default "
                   "values");
              mutable_json = "{}";
            }

            if (!ConfirmationStateManager::GetInstance().FromJson(
                    mutable_json)) {
              BLOG(0, "Failed to parse confirmation state");

              return FailedToMigrate(std::move(callback));
            }

            BLOG(1, "Migrating confirmation state");

            GetAdsClient()->Save(
                kConfirmationsJsonFilename, mutable_json,
                base::BindOnce(
                    [](const std::string& json, InitializeCallback callback,
                       const bool success) {
                      if (!success) {
                        BLOG(0, "Failed to migrate confirmation state");

                        return FailedToMigrate(std::move(callback));
                      }

                      const std::optional<ConfirmationList> confirmations =
                          json::reader::ReadConfirmations(json);
                      if (!confirmations) {
                        // Confirmation queue state does not exist.
                        BLOG(3, "Successfully migrated confirmation state");
                        return SuccessfullyMigrated(std::move(callback));
                      }

                      ConfirmationQueueItemList confirmation_queue_items;
                      for (const auto& confirmation : *confirmations) {
                        const ConfirmationQueueItemInfo
                            confirmation_queue_item =
                                BuildConfirmationQueueItem(
                                    confirmation,
                                    /*process_at=*/base::Time::Now());
                        confirmation_queue_items.push_back(
                            confirmation_queue_item);
                      }

                      database::table::ConfirmationQueue database_table;
                      database_table.Save(
                          confirmation_queue_items,
                          base::BindOnce(
                              [](InitializeCallback callback,
                                 const bool success) {
                                if (!success) {
                                  BLOG(0,
                                       "Failed to migrate confirmation state");

                                  return FailedToMigrate(std::move(callback));
                                }

                                BLOG(3,
                                     "Successfully migrated confirmation "
                                     "state");
                                SuccessfullyMigrated(std::move(callback));
                              },
                              std::move(callback)));
                    },
                    mutable_json, std::move(callback)));
          },
          std::move(callback)));
}

}  // namespace brave_ads
