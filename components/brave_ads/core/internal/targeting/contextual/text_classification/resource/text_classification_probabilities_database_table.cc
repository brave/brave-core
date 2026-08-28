/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_probabilities_database_table.h"

#include <cstddef>
#include <cstdint>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "text_classification_probabilities";

void BindColumnTypes(const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kString,  // segment
      mojom::DBBindColumnType::kDouble,  // page_score
      mojom::DBBindColumnType::kTime     // created_at
  };
}

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const TextClassificationProbabilityMap& probabilities,
                   base::Time created_at) {
  CHECK(mojom_db_action);
  CHECK(!probabilities.empty());

  size_t row_count = 0;

  int32_t index = 0;
  for (const auto& [segment, page_score] : probabilities) {
    BindColumnString(mojom_db_action, index++, segment);
    BindColumnDouble(mojom_db_action, index++, page_score);
    BindColumnTime(mojom_db_action, index++, created_at);

    ++row_count;
  }

  return row_count;
}

void GetAllCallback(
    GetTextClassificationProbabilitiesCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (!IsTransactionSuccessful(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get text classification probabilities");
    return std::move(callback).Run(
        /*success=*/false, /*text_classification_probabilities=*/{});
  }

  CHECK(mojom_db_transaction_result->rows_union);

  // Rows are grouped by `created_at`, newest first, with each group of rows
  // reconstructing one page visit's probability map.
  TextClassificationProbabilityList text_classification_probabilities;
  base::Time last_created_at;
  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    const std::string segment = ColumnString(mojom_db_row, 0);
    const double page_score = ColumnDouble(mojom_db_row, 1);
    const base::Time created_at = ColumnTime(mojom_db_row, 2);

    if (text_classification_probabilities.empty() ||
        created_at != last_created_at) {
      text_classification_probabilities.emplace_back();
      last_created_at = created_at;
    }

    text_classification_probabilities.back().insert({segment, page_score});
  }

  std::move(callback).Run(/*success=*/true,
                          std::move(text_classification_probabilities));
}

std::string BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const TextClassificationProbabilityMap& probabilities,
    base::Time created_at) {
  CHECK(mojom_db_action);
  CHECK(!probabilities.empty());

  const size_t row_count =
      BindColumns(mojom_db_action, probabilities, created_at);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            segment,
            page_score,
            created_at
          ) VALUES $2)",
      {kTableName, BuildBindColumnPlaceholders(/*column_count=*/3U, row_count)},
      nullptr);
}

void Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
            const TextClassificationProbabilityMap& probabilities,
            base::Time created_at) {
  CHECK(mojom_db_transaction);

  if (probabilities.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql =
      BuildInsertSql(mojom_db_action, probabilities, created_at);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

}  // namespace

void TextClassificationProbabilities::Save(
    const TextClassificationProbabilityMap& probabilities,
    base::Time created_at,
    ResultCallback callback) {
  if (probabilities.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  Insert(mojom_db_transaction, probabilities, created_at);

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void TextClassificationProbabilities::PruneToMaximumEntries(
    size_t maximum_entries,
    ResultCallback callback) {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  // Keep rows belonging to the `maximum_entries` most recent distinct
  // `created_at` visits, and delete the rest.
  Execute(mojom_db_transaction, R"(
      DELETE FROM
        $1
      WHERE
        created_at NOT IN (
          SELECT DISTINCT
            created_at
          FROM
            $1
          ORDER BY
            created_at DESC
          LIMIT $2
        ))",
          {kTableName, base::NumberToString(maximum_entries)});

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void TextClassificationProbabilities::DeleteAll(ResultCallback callback) {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
      DELETE FROM
        $1)",
          {kTableName});

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void TextClassificationProbabilities::GetAll(
    GetTextClassificationProbabilitiesCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteQueryWithBindings;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            segment,
            page_score,
            created_at
          FROM
            $1
          ORDER BY
            created_at DESC)",
      {kTableName}, nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 base::BindOnce(&GetAllCallback, std::move(callback)));
}

void TextClassificationProbabilities::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE text_classification_probabilities (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        segment TEXT NOT NULL,
        page_score REAL NOT NULL,
        created_at TIMESTAMP NOT NULL
      ))");

  CreateTableIndex(mojom_db_transaction, kTableName,
                   /*columns=*/{"created_at"});
}

void TextClassificationProbabilities::Migrate(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 59: {
      Create(mojom_db_transaction);
      break;
    }

    default: {
      // No migration needed.
      break;
    }
  }
}

}  // namespace brave_ads::database::table
