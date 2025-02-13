/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/geo_targets_database_table.h"

#include <cstddef>
#include <utility>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "geo_targets";

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const std::map</*campaign_id*/ std::string,
                                  base::flat_set<std::string>>& geo_targets) {
  CHECK(mojom_db_action);
  CHECK(!geo_targets.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& [campaign_id, geo_targets_set] : geo_targets) {
    for (const auto& geo_target : geo_targets_set) {
      BindColumnString(mojom_db_action, index++, campaign_id);
      BindColumnString(mojom_db_action, index++, geo_target);

      ++row_count;
    }
  }

  return row_count;
}

}  // namespace

void GeoTargets::Insert(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    const std::map</*campaign_id*/ std::string, base::flat_set<std::string>>&
        geo_targets) {
  CHECK(mojom_db_transaction);

  if (geo_targets.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, geo_targets);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

std::string GeoTargets::GetTableName() const {
  return kTableName;
}

void GeoTargets::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE geo_targets (
        campaign_id TEXT NOT NULL,
        geo_target TEXT NOT NULL,
        PRIMARY KEY (
          campaign_id,
          geo_target
        ) ON CONFLICT REPLACE
      );)");
}

void GeoTargets::Migrate(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 48: {
      MigrateToV48(mojom_db_transaction);
      break;
    }

    default: {
      // No migration needed.
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void GeoTargets::MigrateToV48(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // It is safe to recreate the table because it will be repopulated after
  // downloading the catalog post-migration. However, after this migration, we
  // should not drop the table as it will store catalog and non-catalog ad units
  // and maintain relationships with other tables.
  DropTable(mojom_db_transaction, GetTableName());
  Create(mojom_db_transaction);
}

std::string GeoTargets::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const std::map</*campaign_id*/ std::string, base::flat_set<std::string>>&
        geo_targets) const {
  CHECK(mojom_db_action);
  CHECK(!geo_targets.empty());

  const size_t row_count = BindColumns(mojom_db_action, geo_targets);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            campaign_id,
            geo_target
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/2, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
