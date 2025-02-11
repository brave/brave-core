/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_SEGMENTS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_SEGMENTS_DATABASE_TABLE_H_

#include <map>
#include <string>

#include "base/containers/flat_set.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads::database::table {

class Segments final : public TableInterface {
 public:
  void Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
              const std::map</*creative_set_id*/ std::string,
                             base::flat_set<std::string>>& segments);

  std::string GetTableName() const override;

  void Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) override;
  void Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
               int to_version) override;

 private:
  void MigrateToV48(const mojom::DBTransactionInfoPtr& mojom_db_transaction);

  std::string BuildInsertSql(
      const mojom::DBActionInfoPtr& mojom_db_action,
      const std::map</*creative_set_id*/ std::string,
                     base::flat_set<std::string>>& segments) const;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_SEGMENTS_DATABASE_TABLE_H_
