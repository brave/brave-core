/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_database_table.h"

#include <cstddef>
#include <cstdint>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "purchase_intent_signal_history";

void BindColumnTypes(const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kString,  // segment
      mojom::DBBindColumnType::kTime,    // created_at
      mojom::DBBindColumnType::kInt      // weight
  };
}

size_t BindColumns(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history) {
  CHECK(mojom_db_action);
  CHECK(!purchase_intent_signal_history.empty());

  size_t row_count = 0;

  int32_t index = 0;
  for (const auto& [segment, history] : purchase_intent_signal_history) {
    for (const auto& item : history) {
      BindColumnString(mojom_db_action, index++, segment);
      BindColumnTime(mojom_db_action, index++, item.at);
      BindColumnInt(mojom_db_action, index++, item.weight);

      ++row_count;
    }
  }

  return row_count;
}

void GetAllCallback(
    GetPurchaseIntentSignalHistoryCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (!IsTransactionSuccessful(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get purchase intent signal history");
    return std::move(callback).Run(/*success=*/false,
                                   /*purchase_intent_signal_history=*/{});
  }

  CHECK(mojom_db_transaction_result->rows_union);

  PurchaseIntentSignalHistoryMap purchase_intent_signal_history;
  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    const std::string segment = ColumnString(mojom_db_row, 0);
    const base::Time at = ColumnTime(mojom_db_row, 1);
    const int weight = ColumnInt(mojom_db_row, 2);

    purchase_intent_signal_history[segment].push_back({at, weight});
  }

  std::move(callback).Run(/*success=*/true,
                          std::move(purchase_intent_signal_history));
}

std::string BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history) {
  CHECK(mojom_db_action);
  CHECK(!purchase_intent_signal_history.empty());

  const size_t row_count =
      BindColumns(mojom_db_action, purchase_intent_signal_history);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            segment,
            created_at,
            weight
          ) VALUES $2)",
      {kTableName, BuildBindColumnPlaceholders(/*column_count=*/3U, row_count)},
      nullptr);
}

void Insert(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history) {
  CHECK(mojom_db_transaction);

  if (purchase_intent_signal_history.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql =
      BuildInsertSql(mojom_db_action, purchase_intent_signal_history);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

}  // namespace

void PurchaseIntentSignalHistory::Save(
    const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history,
    ResultCallback callback) {
  if (purchase_intent_signal_history.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  Insert(mojom_db_transaction, purchase_intent_signal_history);

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void PurchaseIntentSignalHistory::SaveForSegment(
    const std::string& segment,
    const PurchaseIntentSignalHistoryList& history,
    ResultCallback callback) {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  Execute(mojom_db_transaction, R"(
      DELETE FROM
        $1
      WHERE
        segment = '$2')",
          {kTableName, segment});

  if (!history.empty()) {
    Insert(mojom_db_transaction, {{segment, history}});
  }

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void PurchaseIntentSignalHistory::DeleteAll(ResultCallback callback) {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
      DELETE FROM
        $1)",
          {kTableName});

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void PurchaseIntentSignalHistory::GetAll(
    GetPurchaseIntentSignalHistoryCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteQueryWithBindings;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            segment,
            created_at,
            weight
          FROM
            $1)",
      {kTableName}, nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 base::BindOnce(&GetAllCallback, std::move(callback)));
}

void PurchaseIntentSignalHistory::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE purchase_intent_signal_history (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        segment TEXT NOT NULL,
        created_at TIMESTAMP NOT NULL,
        weight INTEGER NOT NULL
      ))");

  CreateTableIndex(mojom_db_transaction, kTableName,
                   /*columns=*/{"segment"});
}

void PurchaseIntentSignalHistory::Migrate(
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
