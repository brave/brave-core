/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration.h"

#include <string>
#include <utility>

#include "absl/types/optional.h"
#include "base/functional/bind.h"
#include "bat/ads/internal/account/transactions/transaction_info.h"
#include "bat/ads/internal/account/transactions/transactions_database_table.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_util.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads::rewards {

namespace {

bool HasMigrated() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(
      prefs::kHasMigratedRewardsState);
}

void FailedToMigrate(InitializeCallback callback) {
  std::move(callback).Run(/*success*/ false);
}

void SuccessfullyMigrated(InitializeCallback callback) {
  AdsClientHelper::GetInstance()->SetBooleanPref(
      prefs::kHasMigratedRewardsState, true);
  std::move(callback).Run(/*success*/ true);
}

void OnMigrate(InitializeCallback callback,
               const bool success,
               const std::string& json) {
  if (!success) {
    // Confirmations state does not exist
    SuccessfullyMigrated(std::move(callback));
    return;
  }

  BLOG(3, "Successfully loaded confirmations state");

  BLOG(3, "Migrating rewards state");

  const absl::optional<TransactionList> transactions =
      BuildTransactionsFromJson(json);
  if (!transactions) {
    BLOG(0, "Failed to parse rewards state");
    FailedToMigrate(std::move(callback));
    return;
  }

  database::table::Transactions database_table;
  database_table.Save(*transactions,
                      base::BindOnce(
                          [](InitializeCallback callback, const bool success) {
                            if (!success) {
                              BLOG(0, "Failed to save rewards state");
                              FailedToMigrate(std::move(callback));
                              return;
                            }

                            BLOG(3, "Successfully migrated rewards state");
                            SuccessfullyMigrated(std::move(callback));
                          },
                          std::move(callback)));
}

}  // namespace

void Migrate(InitializeCallback callback) {
  if (HasMigrated()) {
    std::move(callback).Run(/*success*/ true);
    return;
  }

  BLOG(3, "Loading confirmations state");

  AdsClientHelper::GetInstance()->Load(
      kConfirmationStateFilename,
      base::BindOnce(&OnMigrate, std::move(callback)));
}

}  // namespace ads::rewards
