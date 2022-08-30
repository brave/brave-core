/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration.h"

#include <string>

#include "base/bind.h"
#include "bat/ads/internal/account/transactions/transaction_info.h"
#include "bat/ads/internal/account/transactions/transactions_database_table.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_util.h"
#include "bat/ads/pref_names.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace rewards {

namespace {

bool HasMigrated() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(
      prefs::kHasMigratedRewardsState);
}

void FailedToMigrate(InitializeCallback callback) {
  callback(/*success*/ false);
}

void SuccessfullyMigrated(InitializeCallback callback) {
  AdsClientHelper::GetInstance()->SetBooleanPref(
      prefs::kHasMigratedRewardsState, true);
  callback(/*success*/ true);
}

void OnMigrate(InitializeCallback callback,
               const bool success,
               const std::string& json) {
  if (!success) {
    // Confirmations state does not exist
    SuccessfullyMigrated(callback);
    return;
  }

  BLOG(3, "Successfully loaded confirmations state");

  BLOG(3, "Migrating rewards state");

  const absl::optional<TransactionList> transactions =
      BuildTransactionsFromJson(json);
  if (!transactions) {
    BLOG(0, "Failed to parse rewards state");
    FailedToMigrate(callback);
    return;
  }

  database::table::Transactions database_table;
  database_table.Save(*transactions,
                      base::BindOnce(
                          [](InitializeCallback callback, const bool success) {
                            if (!success) {
                              BLOG(0, "Failed to save rewards state");
                              FailedToMigrate(callback);
                              return;
                            }

                            BLOG(3, "Successfully migrated rewards state");
                            SuccessfullyMigrated(callback);
                          },
                          callback));
}

}  // namespace

void Migrate(InitializeCallback callback) {
  if (HasMigrated()) {
    callback(/*success*/ true);
    return;
  }

  BLOG(3, "Loading confirmations state");

  AdsClientHelper::GetInstance()->Load(kConfirmationStateFilename,
                                       base::BindOnce(&OnMigrate, callback));
}

}  // namespace rewards
}  // namespace ads
