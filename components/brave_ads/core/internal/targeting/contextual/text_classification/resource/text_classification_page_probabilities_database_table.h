/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_PAGE_PROBABILITIES_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_PAGE_PROBABILITIES_DATABASE_TABLE_H_

#include <cstddef>

#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads::database::table {

// One row per classification pass. `TextClassificationProbabilities`
// references a row here via `page_probability_id`.
class TextClassificationPageProbabilities final : public TableInterface {
 public:
  // Takes `mojom_db_transaction` rather than running its own, so callers can
  // commit it together with the related probabilities atomically.
  void Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
              base::Time created_at);

  void DeleteAll(const mojom::DBTransactionInfoPtr& mojom_db_transaction);

  // Adds a delete action to `mojom_db_transaction` that keeps only the
  // `maximum_entries` most recently created rows.
  void PruneToMaximumEntries(
      const mojom::DBTransactionInfoPtr& mojom_db_transaction,
      size_t maximum_entries);

  // TableInterface:
  void Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) override;
  void Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
               int to_version) override;

 private:
  void MigrateToV60(const mojom::DBTransactionInfoPtr& mojom_db_transaction);
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_PAGE_PROBABILITIES_DATABASE_TABLE_H_
