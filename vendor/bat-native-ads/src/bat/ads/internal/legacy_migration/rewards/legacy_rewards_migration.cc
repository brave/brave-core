/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration.h"

#include <string>

#include "bat/ads/internal/account/transactions/transactions_database_table.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_util.h"
#include "bat/ads/pref_names.h"
#include "bat/ads/transaction_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace rewards {

namespace {

bool HasMigrated() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(
      prefs::kHasMigratedRewardsState);
}

void FailedToMigrate(InitializeCallback callback) {
  callback(/* success */ false);
}

void SuccessfullyMigrated(InitializeCallback callback) {
  AdsClientHelper::GetInstance()->SetBooleanPref(
      prefs::kHasMigratedRewardsState, true);
  callback(/* success */ true);
}

}  // namespace

void Migrate(InitializeCallback callback) {
  if (HasMigrated()) {
    callback(/* success */ true);
    return;
  }

  BLOG(3, "Loading confirmations state");

  AdsClientHelper::GetInstance()->Load(
      kConfirmationsFilename, [=](const bool success, const std::string& json) {
        if (!success) {
          // Confirmations state does not exist
          SuccessfullyMigrated(callback);
          return;
        }

        BLOG(3, "Successfully loaded confirmations state");

        BLOG(3, "Migrating rewards state");

        const absl::optional<TransactionList>& transactions_optional =
            BuildTransactionsFromJson(json);
        if (!transactions_optional) {
          BLOG(0, "Failed to parse rewards state");
          FailedToMigrate(callback);
          return;
        }
        const TransactionList& transactions = transactions_optional.value();

        database::table::Transactions database_table;
        database_table.Save(transactions, [=](const bool success) {
          if (!success) {
            BLOG(0, "Failed to save rewards state");
            FailedToMigrate(callback);
            return;
          }

          BLOG(3, "Successfully migrated rewards state");
          SuccessfullyMigrated(callback);
        });
      });
}

}  // namespace rewards
}  // namespace ads
