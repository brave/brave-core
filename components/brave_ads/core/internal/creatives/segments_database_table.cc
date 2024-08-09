/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/segments_database_table.h"

#include <cstddef>
#include <utility>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "segments";

size_t BindColumns(mojom::DBStatementInfo* mojom_statement,
                   const CreativeAdList& creative_ads) {
  CHECK(mojom_statement);
  CHECK(!creative_ads.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindColumnString(mojom_statement, index++, creative_ad.creative_set_id);
    BindColumnString(mojom_statement, index++, creative_ad.segment);

    ++row_count;
  }

  return row_count;
}

}  // namespace

void Segments::Insert(mojom::DBTransactionInfo* mojom_transaction,
                      const CreativeAdList& creative_ads) {
  CHECK(mojom_transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type = mojom::DBStatementInfo::OperationType::kRun;
  mojom_statement->sql = BuildInsertSql(&*mojom_statement, creative_ads);
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

void Segments::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();

  DeleteTable(&*mojom_transaction, GetTableName());

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

std::string Segments::GetTableName() const {
  return kTableName;
}

void Segments::Create(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kExecute;
  mojom_statement->sql =
      R"(
          CREATE TABLE segments (
            creative_set_id TEXT NOT NULL,
            segment TEXT NOT NULL,
            PRIMARY KEY (
              creative_set_id,
              segment
            ) ON CONFLICT REPLACE
          );)";
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

void Segments::Migrate(mojom::DBTransactionInfo* mojom_transaction,
                       const int to_version) {
  CHECK(mojom_transaction);

  switch (to_version) {
    case 43: {
      MigrateToV43(mojom_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void Segments::MigrateToV43(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // We can safely recreate the table because it will be repopulated after
  // downloading the catalog.
  DropTable(mojom_transaction, GetTableName());
  Create(mojom_transaction);
}

std::string Segments::BuildInsertSql(mojom::DBStatementInfo* mojom_statement,
                                     const CreativeAdList& creative_ads) const {
  CHECK(mojom_statement);
  CHECK(!creative_ads.empty());

  const size_t row_count = BindColumns(mojom_statement, creative_ads);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_set_id,
            segment
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/2, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
