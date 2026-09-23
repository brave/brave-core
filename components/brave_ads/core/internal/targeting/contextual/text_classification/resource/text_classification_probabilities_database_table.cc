/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_probabilities_database_table.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

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

// Each page probabilities entry (`kPageProbabilitiesTableName`) has many
// segment scores (`kTableName`), linked by `page_probability_id`.
constexpr char kPageProbabilitiesTableName[] =
    "text_classification_page_probabilities";
constexpr char kTableName[] = "text_classification_probabilities";

void BindColumnTypes(const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kString,  // segment
      mojom::DBBindColumnType::kDouble   // page_score
  };
}

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const TextClassificationProbabilityMap& probabilities) {
  CHECK(mojom_db_action);
  CHECK(!probabilities.empty());

  size_t row_count = 0;

  int32_t index = 0;
  for (const auto& [segment, page_score] : probabilities) {
    BindColumnString(mojom_db_action, index++, segment);
    BindColumnDouble(mojom_db_action, index++, page_score);

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

  TextClassificationProbabilityList text_classification_probabilities;
  std::optional<int64_t> last_page_probability_id;
  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    const int64_t page_probability_id = ColumnInt64(mojom_db_row, 0);
    const std::string segment = ColumnString(mojom_db_row, 1);
    const double page_score = ColumnDouble(mojom_db_row, 2);

    if (last_page_probability_id != page_probability_id) {
      text_classification_probabilities.emplace_back();
      last_page_probability_id = page_probability_id;
    }

    text_classification_probabilities.back().insert({segment, page_score});
  }

  std::move(callback).Run(/*success=*/true,
                          std::move(text_classification_probabilities));
}

std::string BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const TextClassificationProbabilityMap& probabilities) {
  CHECK(mojom_db_action);
  CHECK(!probabilities.empty());

  const size_t row_count = BindColumns(mojom_db_action, probabilities);

  const std::vector<std::string> row_selects(
      row_count, "SELECT ? AS segment, ? AS page_score");

  // Can't use SQLite's last-inserted-id here since it changes after each row
  // within this same multi-row INSERT, so instead look up the parent entry's
  // id directly. Computed once via the `page_probability` CTE, rather than
  // once per row, since it stays the same for every row here.
  return base::ReplaceStringPlaceholders(
      R"(
          WITH page_probability AS (
            SELECT
              MAX(id) AS id
            FROM
              $1
          )
          INSERT INTO $2 (
            page_probability_id,
            segment,
            page_score
          )
          SELECT
            page_probability.id,
            probability.segment,
            probability.page_score
          FROM
            page_probability,
            ($3) AS probability)",
      {kPageProbabilitiesTableName, kTableName,
       base::JoinString(row_selects, " UNION ALL ")},
      nullptr);
}

void Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
            const TextClassificationProbabilityMap& probabilities) {
  CHECK(mojom_db_transaction);

  if (probabilities.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, probabilities);
  BindColumnTypes(mojom_db_action);
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

  page_probabilities_database_table_.Insert(mojom_db_transaction, created_at);
  Insert(mojom_db_transaction, probabilities);

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void TextClassificationProbabilities::PruneToMaximumEntries(
    size_t maximum_entries,
    ResultCallback callback) {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  Execute(mojom_db_transaction, R"(
      DELETE FROM
        $1
      WHERE
        page_probability_id NOT IN (
          SELECT
            id
          FROM
            $2
          ORDER BY
            created_at DESC
          LIMIT $3
        ))",
          {kTableName, kPageProbabilitiesTableName,
           base::NumberToString(maximum_entries)});

  page_probabilities_database_table_.PruneToMaximumEntries(mojom_db_transaction,
                                                           maximum_entries);

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
  page_probabilities_database_table_.DeleteAll(mojom_db_transaction);

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
            page_probability.id,
            probability.segment,
            probability.page_score
          FROM
            $1 AS probability
            INNER JOIN $2 AS page_probability
              ON page_probability.id = probability.page_probability_id
          ORDER BY
            page_probability.created_at DESC,
            page_probability.id DESC)",
      {kTableName, kPageProbabilitiesTableName}, nullptr);
  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kInt64,   // page_probability.id
      mojom::DBBindColumnType::kString,  // probability.segment
      mojom::DBBindColumnType::kDouble   // probability.page_score
  };
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
        page_probability_id INTEGER NOT NULL REFERENCES
            text_classification_page_probabilities (id),
        segment TEXT NOT NULL,
        page_score REAL NOT NULL
      ))");

  CreateTableIndex(mojom_db_transaction, kTableName,
                   /*columns=*/{"page_probability_id"});
}

void TextClassificationProbabilities::Migrate(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 59: {
      MigrateToV59(mojom_db_transaction);
      break;
    }

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

void TextClassificationProbabilities::MigrateToV59(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Kept separate from `Create` so `MigrateToV60` always converts from this
  // same flat shape, regardless of which version an install started at.
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

void TextClassificationProbabilities::MigrateToV60(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // `TextClassificationPageProbabilities::Migrate` has already renamed the
  // old table to `text_classification_probabilities_temp` and rebuilt
  // `text_classification_page_probabilities` from it.
  //
  // Visits that already shared a `created_at` can't be told apart here
  // either, but that ambiguity predates this migration.
  Create(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      INSERT INTO text_classification_probabilities (
        page_probability_id,
        segment,
        page_score
      )
      SELECT
        page_probability.id,
        temp.segment,
        temp.page_score
      FROM
        text_classification_probabilities_temp AS temp
        INNER JOIN text_classification_page_probabilities AS page_probability
          ON page_probability.created_at = temp.created_at)");

  DropTable(mojom_db_transaction, "text_classification_probabilities_temp");
}

}  // namespace brave_ads::database::table
