/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"

#include <cstddef>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "transactions";

void BindColumnTypes(const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kString,  // id
      mojom::DBBindColumnType::kTime,    // created_at
      mojom::DBBindColumnType::kString,  // creative_instance_id
      mojom::DBBindColumnType::kDouble,  // value
      mojom::DBBindColumnType::kString,  // segment
      mojom::DBBindColumnType::kString,  // ad_type
      mojom::DBBindColumnType::kString,  // confirmation_type
      mojom::DBBindColumnType::kTime     // reconciled_at
  };
}

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const TransactionList& transactions) {
  CHECK(mojom_db_action);
  CHECK(!transactions.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& transaction : transactions) {
    if (!transaction.IsValid()) {
      base::debug::DumpWithoutCrashing();
      BLOG(0, "Invalid transaction");
      continue;
    }

    BindColumnString(mojom_db_action, index++, transaction.id);
    BindColumnTime(mojom_db_action, index++,
                   transaction.created_at.value_or(base::Time()));
    BindColumnString(mojom_db_action, index++,
                     transaction.creative_instance_id);
    BindColumnDouble(mojom_db_action, index++, transaction.value);
    BindColumnString(mojom_db_action, index++, transaction.segment);
    BindColumnString(mojom_db_action, index++, ToString(transaction.ad_type));
    BindColumnString(mojom_db_action, index++,
                     ToString(transaction.confirmation_type));
    BindColumnTime(mojom_db_action, index++,
                   transaction.reconciled_at.value_or(base::Time()));

    ++row_count;
  }

  return row_count;
}

TransactionInfo FromMojomRow(const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  TransactionInfo transaction;

  transaction.id = ColumnString(mojom_db_row, 0);
  const base::Time created_at = ColumnTime(mojom_db_row, 1);
  if (!created_at.is_null()) {
    transaction.created_at = created_at;
  }
  transaction.creative_instance_id = ColumnString(mojom_db_row, 2);
  transaction.value = ColumnDouble(mojom_db_row, 3);
  transaction.segment = ColumnString(mojom_db_row, 4);
  transaction.ad_type = ToMojomAdType(ColumnString(mojom_db_row, 5));
  transaction.confirmation_type =
      ToMojomConfirmationType(ColumnString(mojom_db_row, 6));
  const base::Time reconciled_at = ColumnTime(mojom_db_row, 7);
  if (!reconciled_at.is_null()) {
    transaction.reconciled_at = reconciled_at;
  }

  return transaction;
}

void GetCallback(
    GetTransactionsCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (IsError(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get transactions");
    return std::move(callback).Run(/*success=*/false, /*transactions=*/{});
  }

  CHECK(mojom_db_transaction_result->rows_union);

  TransactionList transactions;

  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    const TransactionInfo transaction = FromMojomRow(mojom_db_row);
    if (!transaction.IsValid()) {
      base::debug::DumpWithoutCrashing();
      BLOG(0, "Invalid transaction");
      continue;
    }

    transactions.push_back(transaction);
  }

  std::move(callback).Run(/*success=*/true, transactions);
}

void MigrateToV35(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Optimize database query for `GetForDateRange`.
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"transactions",
                   /*columns=*/{"created_at"});
}

void MigrateToV40(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Delete legacy transactions with an undefined `creative_instance_id`,
  // `segment` or `ad_type`.
  Execute(mojom_db_transaction, R"(
      DELETE FROM
        transactions
      WHERE
        COALESCE(creative_instance_id, '') = ''
        OR COALESCE(segment, '') = ''
        OR ad_type = '';)");

  // Create a temporary table:
  //   - with a new `creative_instance_id` column constraint.
  //   - with a new `segment` column constraint.
  //   - with a new `reconciled_at` default value.
  Execute(mojom_db_transaction, R"(
      CREATE TABLE transactions_temp (
        id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        created_at TIMESTAMP NOT NULL,
        creative_instance_id TEXT NOT NULL,
        value DOUBLE NOT NULL,
        segment TEXT NOT NULL,
        ad_type TEXT NOT NULL,
        confirmation_type TEXT NOT NULL,
        reconciled_at TIMESTAMP DEFAULT 0
      );)");

  // Copy legacy columns to the temporary table, drop the legacy table,
  // rename the temporary table and create an index.
  const std::vector<std::string> columns = {
      "id",      "created_at", "creative_instance_id", "value",
      "segment", "ad_type",    "confirmation_type",    "reconciled_at"};

  CopyTableColumns(mojom_db_transaction, "transactions", "transactions_temp",
                   columns, /*should_drop=*/true);

  RenameTable(mojom_db_transaction, "transactions_temp", "transactions");

  // Optimize database query for `GetForDateRange`.
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"transactions",
                   /*columns=*/{"created_at"});
}

void MigrateToV43(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Optimize database query for `Reconcile`.
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"transactions",
                   /*columns=*/{"reconciled_at"});
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"transactions",
                   /*columns=*/{"id"});
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"transactions",
                   /*columns=*/{"creative_instance_id"});
}

}  // namespace

void Transactions::Save(const TransactionList& transactions,
                        ResultCallback callback) {
  if (transactions.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  Insert(mojom_db_transaction, transactions);

  RunDBTransaction(std::move(mojom_db_transaction), std::move(callback));
}

void Transactions::GetForDateRange(base::Time from_time,
                                   base::Time to_time,
                                   GetTransactionsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            id,
            created_at,
            creative_instance_id,
            value,
            segment,
            ad_type,
            confirmation_type,
            reconciled_at
          FROM
            $1
          WHERE
            created_at BETWEEN $2 AND $3;)",
      {GetTableName(), TimeToSqlValueAsString(from_time),
       TimeToSqlValueAsString(to_time)},
      nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunDBTransaction(std::move(mojom_db_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void Transactions::Reconcile(const PaymentTokenList& payment_tokens,
                             ResultCallback callback) const {
  std::vector<std::string> transaction_ids;
  for (const auto& payment_token : payment_tokens) {
    transaction_ids.push_back(payment_token.transaction_id);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kRunStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          UPDATE
            $1
          SET
            reconciled_at = $2
          WHERE
            reconciled_at == 0
            AND (
              id IN $3
              OR creative_instance_id IN $4
            );)",
      {GetTableName(), TimeToSqlValueAsString(base::Time::Now()),
       BuildBindColumnPlaceholder(
           /*column_count=*/transaction_ids.size()),
       BuildBindColumnPlaceholder(/*column_count=*/1)},
      nullptr);

  int index = 0;
  for (const auto& transaction_id : transaction_ids) {
    BindColumnString(mojom_db_action, index, transaction_id);
    ++index;
  }

  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunDBTransaction(std::move(mojom_db_transaction), std::move(callback));
}

void Transactions::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
            DELETE FROM
              $1
            WHERE
              reconciled_at != 0
            AND created_at <= $2;)",
          {GetTableName(),
           TimeToSqlValueAsString(base::Time::Now() - base::Days(90))});

  RunDBTransaction(std::move(mojom_db_transaction), std::move(callback));
}

std::string Transactions::GetTableName() const {
  return kTableName;
}

void Transactions::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE transactions (
        id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        created_at TIMESTAMP NOT NULL,
        creative_instance_id TEXT NOT NULL,
        value DOUBLE NOT NULL,
        segment TEXT NOT NULL,
        ad_type TEXT NOT NULL,
        confirmation_type TEXT NOT NULL,
        reconciled_at TIMESTAMP DEFAULT 0
      );)");

  // Optimize database query for `GetForDateRange` from schema 35 and 40.
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"created_at"});

  // Optimize database query for `Reconcile` from schema 43.
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"reconciled_at"});
  CreateTableIndex(mojom_db_transaction, GetTableName(), /*columns=*/{"id"});
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"creative_instance_id"});
}

void Transactions::Migrate(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 35: {
      MigrateToV35(mojom_db_transaction);
      break;
    }

    case 40: {
      MigrateToV40(mojom_db_transaction);
      break;
    }

    case 43: {
      MigrateToV43(mojom_db_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void Transactions::Insert(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    const TransactionList& transactions) {
  CHECK(mojom_db_transaction);

  if (transactions.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kRunStatement;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, transactions);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

std::string Transactions::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const TransactionList& transactions) const {
  CHECK(mojom_db_action);
  CHECK(!transactions.empty());

  const size_t row_count = BindColumns(mojom_db_action, transactions);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            id,
            created_at,
            creative_instance_id,
            value,
            segment,
            ad_type,
            confirmation_type,
            reconciled_at
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/8, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
