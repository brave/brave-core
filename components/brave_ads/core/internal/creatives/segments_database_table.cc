/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/segments_database_table.h"

#include <cstddef>
#include <cstdint>
#include <utility>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "segments";

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const std::map</*creative_set_id*/ std::string,
                                  base::flat_set<std::string>>& segments) {
  CHECK(mojom_db_action);
  CHECK(!segments.empty());

  size_t row_count = 0;

  int32_t index = 0;
  for (const auto& [creative_set_id, segments_set] : segments) {
    for (const auto& segment : segments_set) {
      BindColumnString(mojom_db_action, index++, creative_set_id);
      BindColumnString(mojom_db_action, index++, segment);

      ++row_count;
    }
  }

  return row_count;
}

}  // namespace

void Segments::Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                      const std::map</*creative_set_id*/ std::string,
                                     base::flat_set<std::string>>& segments) {
  CHECK(mojom_db_transaction);

  if (segments.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, segments);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

std::string Segments::GetTableName() const {
  return kTableName;
}

void Segments::Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE segments (
        creative_set_id TEXT NOT NULL,
        segment TEXT NOT NULL,
        PRIMARY KEY (
          creative_set_id,
          segment
        ) ON CONFLICT REPLACE
      ))");
}

void Segments::Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
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

void Segments::MigrateToV48(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // It is safe to recreate the table because it will be repopulated after
  // downloading the catalog post-migration. However, after this migration, we
  // should not drop the table as it will store catalog and non-catalog ad units
  // and maintain relationships with other tables.
  DropTable(mojom_db_transaction, GetTableName());
  Create(mojom_db_transaction);
}

std::string Segments::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const std::map</*creative_set_id*/ std::string,
                   base::flat_set<std::string>>& segments) const {
  CHECK(mojom_db_action);
  CHECK(!segments.empty());

  const size_t row_count = BindColumns(mojom_db_action, segments);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_set_id,
            segment
          ) VALUES $2)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/2, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
