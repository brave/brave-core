/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_DAYPARTS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_DAYPARTS_DATABASE_TABLE_H_

#include <map>
#include <string>

#include "base/containers/flat_set.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

struct CreativeDaypartInfo;

namespace database::table {

class Dayparts final : public TableInterface {
 public:
  void Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
              const std::map</*campaign_id*/ std::string,
                             base::flat_set<CreativeDaypartInfo>>& dayparts);

  std::string GetTableName() const override;

  void Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) override;
  void Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
               int to_version) override;

 private:
  void MigrateToV48(const mojom::DBTransactionInfoPtr& mojom_db_transaction);

  std::string BuildInsertSql(
      const mojom::DBActionInfoPtr& mojom_db_action,
      const std::map</*campaign_id*/ std::string,
                     base::flat_set<CreativeDaypartInfo>>& dayparts) const;
};

}  // namespace database::table

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_DAYPARTS_DATABASE_TABLE_H_
