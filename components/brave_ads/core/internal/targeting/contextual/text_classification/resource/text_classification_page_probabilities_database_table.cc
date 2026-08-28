/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_page_probabilities_database_table.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "text_classification_page_probabilities";

std::string BuildInsertSql(const mojom::DBActionInfoPtr& mojom_db_action,
                           base::Time created_at) {
  CHECK(mojom_db_action);

  BindColumnTime(mojom_db_action, /*index=*/0, created_at);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            created_at
          ) VALUES (?))",
      {kTableName}, nullptr);
}

}  // namespace

void TextClassificationPageProbabilities::Insert(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    base::Time created_at) {
  CHECK(mojom_db_transaction);

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, created_at);
  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kTime  // created_at
  };
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

void TextClassificationPageProbabilities::DeleteAll(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      DELETE FROM
        $1)",
          {kTableName});
}

void TextClassificationPageProbabilities::PruneToMaximumEntries(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    size_t maximum_entries) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      DELETE FROM
        $1
      WHERE
        id NOT IN (
          SELECT
            id
          FROM
            $1
          ORDER BY
            created_at DESC
          LIMIT $2
        ))",
          {kTableName, base::NumberToString(maximum_entries)});
}

void TextClassificationPageProbabilities::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE text_classification_page_probabilities (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        created_at TIMESTAMP NOT NULL
      ))");

  CreateTableIndex(mojom_db_transaction, kTableName,
                   /*columns=*/{"created_at"});
}

void TextClassificationPageProbabilities::Migrate(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 60: {
      MigrateToV60(mojom_db_transaction);
      break;
    }

    default: {
      // No migration needed.
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void TextClassificationPageProbabilities::MigrateToV60(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Renamed rather than dropped, since `TextClassificationProbabilities`
  // still needs to read the old rows to backfill its own table.
  RenameTable(mojom_db_transaction, "text_classification_probabilities",
              "text_classification_probabilities_temp");

  Create(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      INSERT INTO text_classification_page_probabilities (
        created_at
      )
      SELECT DISTINCT
        created_at
      FROM
        text_classification_probabilities_temp)");
}

}  // namespace brave_ads::database::table
