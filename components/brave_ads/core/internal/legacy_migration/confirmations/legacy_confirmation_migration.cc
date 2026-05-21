/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_database_table.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_database_table.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_confirmation_queue_builder.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_confirmation_tokens_json_parser.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_confirmations_json_parser.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_payment_tokens_json_parser.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/legacy_migration_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

namespace {

constexpr char kConfirmationsJsonFilename[] = "confirmations.json";

void SuccessfullyMigrated(ResultCallback callback) {
  MaybeDeleteFile(kConfirmationsJsonFilename);
  std::move(callback).Run(/*success=*/true);
}

void MigrationCallback(ResultCallback callback,
                       const std::vector<bool>& results) {
  for (const bool success : results) {
    if (!success) {
      // Leave `kConfirmationsJsonFilename` on disk so migration retries on the
      // next startup. The confirmation server uses `transaction_id` as an
      // idempotency key, so duplicate queue items submitted on retry are safe.
      BLOG(0, "Failed to migrate confirmation state");
      return std::move(callback).Run(/*success=*/false);
    }
  }

  BLOG(3, "Successfully migrated confirmation state");
  SuccessfullyMigrated(std::move(callback));
}

void LoadConfirmationStateCallback(std::optional<WalletInfo> wallet,
                                   ResultCallback callback,
                                   const std::optional<std::string>& json) {
  if (!json) {
    // Confirmation state does not exist. Either the browser is starting
    // fresh or the file was already deleted by a prior successful migration.
    return std::move(callback).Run(/*success=*/true);
  }

  if (!base::JSONReader::ReadDict(*json, base::JSON_PARSE_RFC)) {
    // The confirmation state is corrupted. Treat as empty and remove the file
    // so migration does not loop.
    BLOG(0, "Confirmation state is corrupted, resetting to default values");
    return SuccessfullyMigrated(std::move(callback));
  }

  BLOG(1, "Migrating confirmation state from " << kConfirmationsJsonFilename);

  // Without a wallet, confirmation and payment tokens cannot be verified or
  // redeemed, so only the confirmation queue is migrated.
  const auto barrier_callback = base::BarrierCallback<bool>(
      wallet ? 3U : 1U,
      base::BindOnce(&MigrationCallback, std::move(callback)));

  const ConfirmationQueueItemList confirmation_queue_items =
      BuildConfirmationQueueItems(json::reader::ParseConfirmations(*json));
  database::table::ConfirmationQueue confirmation_queue_database_table;
  confirmation_queue_database_table.Save(confirmation_queue_items,
                                         barrier_callback);

  if (!wallet) {
    return;
  }

  const ConfirmationTokenList confirmation_tokens =
      json::reader::ParseConfirmationTokens(*json, *wallet).value_or({});
  database::table::ConfirmationTokens confirmation_tokens_database_table;
  confirmation_tokens_database_table.Save(confirmation_tokens,
                                          barrier_callback);

  const PaymentTokenList payment_tokens =
      json::reader::ParsePaymentTokens(*json).value_or({});
  database::table::PaymentTokens payment_tokens_database_table;
  payment_tokens_database_table.Save(payment_tokens, barrier_callback);
}

}  // namespace

void MigrateConfirmationState(std::optional<WalletInfo> wallet,
                              ResultCallback callback) {
  GetAdsClient().Load(kConfirmationsJsonFilename,
                      base::BindOnce(&LoadConfirmationStateCallback,
                                     std::move(wallet), std::move(callback)));
}

}  // namespace brave_ads
