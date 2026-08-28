/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_purchase_intent_signal_history_json_parser.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_text_classification_probabilities_json_parser.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/legacy_migration_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_database_table.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_probabilities_database_table.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

namespace brave_ads {

namespace {

void SuccessfullyMigrated(ResultCallback callback) {
  MaybeDeleteFile(kClientJsonFilename);
  std::move(callback).Run(/*success=*/true);
}

void MigrationCallback(ResultCallback callback,
                       const std::vector<bool>& results) {
  for (const bool success : results) {
    if (!success) {
      // Leave `kClientJsonFilename` on disk so migration retries on the next
      // startup.
      BLOG(0, "Failed to migrate client state");
      return std::move(callback).Run(/*success=*/false);
    }
  }

  BLOG(3, "Successfully migrated client state");
  SuccessfullyMigrated(std::move(callback));
}

void LoadClientStateCallback(ResultCallback callback,
                             const std::optional<std::string>& json) {
  if (!json) {
    // Client state does not exist. Either the browser is starting fresh or
    // the file was already deleted by a prior successful migration.
    return std::move(callback).Run(/*success=*/true);
  }

  if (!base::JSONReader::ReadDict(*json, base::JSON_PARSE_RFC)) {
    // The client state is corrupted. Treat as empty and remove the file so
    // migration does not loop.
    BLOG(0, "Client state is corrupted, resetting to default values");
    return SuccessfullyMigrated(std::move(callback));
  }

  BLOG(1, "Migrating client state from " << kClientJsonFilename);

  const PurchaseIntentSignalHistoryMap purchase_intent_signal_history =
      json::reader::ParsePurchaseIntentSignalHistory(*json).value_or({});

  const TextClassificationProbabilityList text_classification_probabilities =
      json::reader::ParseTextClassificationProbabilities(*json).value_or({});

  const auto barrier_callback = base::BarrierCallback<bool>(
      /*num_callbacks=*/1 + text_classification_probabilities.size(),
      base::BindOnce(&MigrationCallback, std::move(callback)));

  database::table::PurchaseIntentSignalHistory
      purchase_intent_signal_history_database_table;
  purchase_intent_signal_history_database_table.Save(
      purchase_intent_signal_history, barrier_callback);

  database::table::TextClassificationProbabilities
      text_classification_probabilities_database_table;

  // Assign each visit a strictly decreasing synthetic timestamp so the
  // original newest-first ordering can be reconstructed from `created_at`,
  // since the legacy format did not record a timestamp per visit.
  const base::Time now = base::Time::Now();
  int64_t index = 0;
  for (const auto& probabilities : text_classification_probabilities) {
    text_classification_probabilities_database_table.Save(
        probabilities, now - base::Milliseconds(index++), barrier_callback);
  }
}

}  // namespace

void MigrateClientState(ResultCallback callback) {
  GetAdsClient().Load(
      kClientJsonFilename,
      base::BindOnce(&LoadClientStateCallback, std::move(callback)));
}

}  // namespace brave_ads
