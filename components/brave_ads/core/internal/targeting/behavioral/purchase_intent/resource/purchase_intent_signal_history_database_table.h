/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_SIGNAL_HISTORY_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_SIGNAL_HISTORY_DATABASE_TABLE_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads::database::table {

using GetPurchaseIntentSignalHistoryCallback = base::OnceCallback<void(
    bool success,
    const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history)>;

// Persists purchase intent signal history keyed by segment. History is
// loaded from this table on startup to populate the in-memory cache, and
// written here on every append.
class PurchaseIntentSignalHistory final : public TableInterface {
 public:
  void Save(
      const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history,
      ResultCallback callback);

  // Replaces `segment`'s history with `history` in a single transaction.
  void SaveForSegment(const std::string& segment,
                      const PurchaseIntentSignalHistoryList& history,
                      ResultCallback callback);

  void DeleteAll(ResultCallback callback);

  void GetAll(GetPurchaseIntentSignalHistoryCallback callback) const;

  // TableInterface:
  void Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) override;
  void Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
               int to_version) override;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_SIGNAL_HISTORY_DATABASE_TABLE_H_
