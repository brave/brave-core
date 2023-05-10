/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::rewards {

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
               const absl::optional<std::string>& json) {
  if (!json) {
    // Confirmations state does not exist
    return SuccessfullyMigrated(std::move(callback));
  }

  BLOG(3, "Migrating rewards state");

  const absl::optional<TransactionList> transactions =
      BuildTransactionsFromJson(*json);
  if (!transactions) {
    BLOG(0, "Failed to parse rewards state");
    return FailedToMigrate(std::move(callback));
  }

  database::table::Transactions database_table;
  database_table.Save(*transactions,
                      base::BindOnce(
                          [](InitializeCallback callback, const bool success) {
                            if (!success) {
                              BLOG(0, "Failed to save rewards state");
                              return FailedToMigrate(std::move(callback));
                            }

                            BLOG(3, "Successfully migrated rewards state");
                            SuccessfullyMigrated(std::move(callback));
                          },
                          std::move(callback)));
}

}  // namespace

void Migrate(InitializeCallback callback) {
  if (HasMigrated()) {
    return std::move(callback).Run(/*success*/ true);
  }

  AdsClientHelper::GetInstance()->Load(
      kConfirmationStateFilename,
      base::BindOnce(&OnMigrate, std::move(callback)));
}

}  // namespace brave_ads::rewards
