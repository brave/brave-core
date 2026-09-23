/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_PROBABILITIES_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_PROBABILITIES_DATABASE_TABLE_H_

#include <cstddef>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_page_probabilities_database_table.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_types.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads::database::table {

using GetTextClassificationProbabilitiesCallback =
    base::OnceCallback<void(bool success,
                            const TextClassificationProbabilityList&
                                text_classification_probabilities)>;

// Persists the history of text classification probabilities computed for
// visited pages. History is loaded from this table on startup to populate
// the in-memory cache, and written here on every append.
class TextClassificationProbabilities final : public TableInterface {
 public:
  // Saves one page visit's worth of probabilities, all sharing `created_at`
  // so the original visit ordering can be reconstructed by `GetAll`.
  void Save(const TextClassificationProbabilityMap& probabilities,
            base::Time created_at,
            ResultCallback callback);

  // Deletes probabilities for the oldest visits beyond `maximum_entries`.
  void PruneToMaximumEntries(size_t maximum_entries, ResultCallback callback);

  void DeleteAll(ResultCallback callback);

  void GetAll(GetTextClassificationProbabilitiesCallback callback) const;

  // TableInterface:
  void Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) override;
  void Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
               int to_version) override;

 private:
  void MigrateToV59(const mojom::DBTransactionInfoPtr& mojom_db_transaction);
  void MigrateToV60(const mojom::DBTransactionInfoPtr& mojom_db_transaction);

  TextClassificationPageProbabilities page_probabilities_database_table_;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_PROBABILITIES_DATABASE_TABLE_H_
