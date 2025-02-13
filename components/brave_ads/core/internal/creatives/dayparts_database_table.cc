/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/dayparts_database_table.h"

#include <cstddef>
#include <utility>

#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "dayparts";

size_t BindColumns(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const std::map</*campaign_id*/ std::string,
                   base::flat_set<CreativeDaypartInfo>>& dayparts) {
  CHECK(mojom_db_action);
  CHECK(!dayparts.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& [campaign_id, dayparts_set] : dayparts) {
    for (const auto& daypart : dayparts_set) {
      BindColumnString(mojom_db_action, index++, campaign_id);
      BindColumnString(mojom_db_action, index++, daypart.days_of_week);
      BindColumnInt(mojom_db_action, index++, daypart.start_minute);
      BindColumnInt(mojom_db_action, index++, daypart.end_minute);

      ++row_count;
    }
  }

  return row_count;
}

}  // namespace

void Dayparts::Insert(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    const std::map</*campaign_id*/ std::string,
                   base::flat_set<CreativeDaypartInfo>>& dayparts) {
  CHECK(mojom_db_transaction);

  if (dayparts.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, dayparts);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

std::string Dayparts::GetTableName() const {
  return kTableName;
}

void Dayparts::Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE dayparts (
        campaign_id TEXT NOT NULL,
        days_of_week TEXT NOT NULL,
        start_minute INT NOT NULL,
        end_minute INT NOT NULL,
        PRIMARY KEY (
          campaign_id,
          days_of_week,
          start_minute,
          end_minute
        ) ON CONFLICT REPLACE
      ))");
}

void Dayparts::Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
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

void Dayparts::MigrateToV48(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // It is safe to recreate the table because it will be repopulated after
  // downloading the catalog post-migration. However, after this migration, we
  // should not drop the table as it will store catalog and non-catalog ad units
  // and maintain relationships with other tables.
  DropTable(mojom_db_transaction, GetTableName());
  Create(mojom_db_transaction);
}

std::string Dayparts::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const std::map</*campaign_id*/ std::string,
                   base::flat_set<CreativeDaypartInfo>>& dayparts) const {
  CHECK(mojom_db_action);
  CHECK(!dayparts.empty());

  const size_t row_count = BindColumns(mojom_db_action, dayparts);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            campaign_id,
            days_of_week,
            start_minute,
            end_minute
          ) VALUES $2)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/4, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
