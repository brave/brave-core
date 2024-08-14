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
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "transactions";

void BindColumnTypes(mojom::DBStatementInfo* const mojom_statement) {
  CHECK(mojom_statement);

  mojom_statement->bind_column_types = {
      mojom::DBBindColumnType::kString,  // id
      mojom::DBBindColumnType::kInt64,   // created_at
      mojom::DBBindColumnType::kString,  // creative_instance_id
      mojom::DBBindColumnType::kDouble,  // value
      mojom::DBBindColumnType::kString,  // segment
      mojom::DBBindColumnType::kString,  // ad_type
      mojom::DBBindColumnType::kString,  // confirmation_type
      mojom::DBBindColumnType::kInt64    // reconciled_at
  };
}

size_t BindColumns(mojom::DBStatementInfo* mojom_statement,
                   const TransactionList& transactions) {
  CHECK(mojom_statement);
  CHECK(!transactions.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& transaction : transactions) {
    if (!transaction.IsValid()) {
      base::debug::DumpWithoutCrashing();
      BLOG(0, "Invalid transaction");
      continue;
    }

    BindColumnString(mojom_statement, index++, transaction.id);
    BindColumnInt64(mojom_statement, index++,
                    ToChromeTimestampFromTime(
                        transaction.created_at.value_or(base::Time())));
    BindColumnString(mojom_statement, index++,
                     transaction.creative_instance_id);
    BindColumnDouble(mojom_statement, index++, transaction.value);
    BindColumnString(mojom_statement, index++, transaction.segment);
    BindColumnString(mojom_statement, index++, ToString(transaction.ad_type));
    BindColumnString(mojom_statement, index++,
                     ToString(transaction.confirmation_type));
    BindColumnInt64(mojom_statement, index++,
                    ToChromeTimestampFromTime(
                        transaction.reconciled_at.value_or(base::Time())));

    ++row_count;
  }

  return row_count;
}

TransactionInfo FromMojomRow(const mojom::DBRowInfo* const mojom_row) {
  CHECK(mojom_row);

  TransactionInfo transaction;

  transaction.id = ColumnString(mojom_row, 0);
  const base::Time created_at =
      ToTimeFromChromeTimestamp(ColumnInt64(mojom_row, 1));
  if (!created_at.is_null()) {
    transaction.created_at = created_at;
  }
  transaction.creative_instance_id = ColumnString(mojom_row, 2);
  transaction.value = ColumnDouble(mojom_row, 3);
  transaction.segment = ColumnString(mojom_row, 4);
  transaction.ad_type = ToAdType(ColumnString(mojom_row, 5));
  transaction.confirmation_type =
      ToConfirmationType(ColumnString(mojom_row, 6));
  const base::Time reconciled_at =
      ToTimeFromChromeTimestamp(ColumnInt64(mojom_row, 7));
  if (!reconciled_at.is_null()) {
    transaction.reconciled_at = reconciled_at;
  }

  return transaction;
}

void GetCallback(GetTransactionsCallback callback,
                 mojom::DBStatementResultInfoPtr mojom_statement_result) {
  if (!mojom_statement_result ||
      mojom_statement_result->result_code !=
          mojom::DBStatementResultInfo::ResultCode::kSuccess) {
    BLOG(0, "Failed to get transactions");

    return std::move(callback).Run(/*success=*/false, /*transactions=*/{});
  }

  CHECK(mojom_statement_result->rows_union);

  TransactionList transactions;

  for (const auto& mojom_row : mojom_statement_result->rows_union->get_rows()) {
    const TransactionInfo transaction = FromMojomRow(&*mojom_row);
    if (!transaction.IsValid()) {
      base::debug::DumpWithoutCrashing();
      BLOG(0, "Invalid transaction");
      continue;
    }

    transactions.push_back(transaction);
  }

  std::move(callback).Run(/*success=*/true, transactions);
}

void MigrateToV18(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  Execute(mojom_transaction, R"(
      CREATE TABLE transactions (
        id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        created_at TIMESTAMP NOT NULL,
        creative_instance_id TEXT,
        value DOUBLE NOT NULL,
        ad_type TEXT NOT NULL,
        confirmation_type TEXT NOT NULL,
        reconciled_at TIMESTAMP
      );)");
}

void MigrateToV26(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Create a temporary table:
  //   - with a new `segment` column.
  Execute(mojom_transaction, R"(
      CREATE TABLE transactions_temp (
        id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        created_at TIMESTAMP NOT NULL,
        creative_instance_id TEXT,
        value DOUBLE NOT NULL,
        segment TEXT,
        ad_type TEXT NOT NULL,
        confirmation_type TEXT NOT NULL,
        reconciled_at TIMESTAMP
      );)");

  // Copy legacy columns to the temporary table, drop the legacy table, rename
  // the temporary table and create an index.
  const std::vector<std::string> columns = {
      "id",      "created_at",        "creative_instance_id", "value",
      "ad_type", "confirmation_type", "reconciled_at"};

  CopyTableColumns(mojom_transaction, "transactions", "transactions_temp",
                   columns, /*should_drop=*/true);

  RenameTable(mojom_transaction, "transactions_temp", "transactions");
}

void MigrateToV29(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

    // Migrate `created_at` column from an Epoch timestamp to a WebKit/Chrome
    // timestamp.
  Execute(mojom_transaction, R"(
      UPDATE
        transactions
      SET
        created_at = (
          CAST(created_at AS INT64) + 11644473600
        ) * 1000000;)");

  // Migrate `reconciled_at` column from an Epoch timestamp to a WebKit/Chrome
  // timestamp.
  Execute(mojom_transaction, R"(
      UPDATE
        transactions
      SET
        reconciled_at = (
          CAST(reconciled_at AS INT64) + 11644473600
        ) * 1000000
      WHERE
        reconciled_at != 0;)");
}

void MigrateToV32(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Migrate `confirmation_type` from 'saved' to 'bookmark'.
  Execute(mojom_transaction, R"(
      UPDATE
        transactions
      SET
        confirmation_type = 'bookmark'
      WHERE
        confirmation_type == 'saved';)");
}

void MigrateToV35(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Optimize database query for `GetForDateRange`.
  CreateTableIndex(mojom_transaction, /*table_name=*/"transactions",
                   /*columns=*/{"created_at"});
}

void MigrateToV39(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Delete legacy transactions with an undefined `created_at` timestamp.
  Execute(mojom_transaction, R"(
      DELETE FROM
        transactions
      WHERE
        created_at == 0;)");
}

void MigrateToV40(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Delete legacy transactions with an undefined `creative_instance_id`,
  // `segment` or `ad_type`.
  Execute(mojom_transaction, R"(
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
  Execute(mojom_transaction, R"(
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

  CopyTableColumns(mojom_transaction, "transactions", "transactions_temp",
                   columns, /*should_drop=*/true);

  RenameTable(mojom_transaction, "transactions_temp", "transactions");

  // Optimize database query for `GetForDateRange`.
  CreateTableIndex(mojom_transaction, /*table_name=*/"transactions",
                   /*columns=*/{"created_at"});
}

void MigrateToV43(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Optimize database query for `Reconcile`.
  CreateTableIndex(mojom_transaction, /*table_name=*/"transactions",
                   /*columns=*/{"reconciled_at"});
  CreateTableIndex(mojom_transaction, /*table_name=*/"transactions",
                   /*columns=*/{"id"});
  CreateTableIndex(mojom_transaction, /*table_name=*/"transactions",
                   /*columns=*/{"creative_instance_id"});
}

}  // namespace

void Transactions::Save(const TransactionList& transactions,
                        ResultCallback callback) {
  if (transactions.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();

  Insert(&*mojom_transaction, transactions);

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void Transactions::GetForDateRange(const base::Time from_time,
                                   const base::Time to_time,
                                   GetTransactionsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kStep;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
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
      {GetTableName(),
       base::NumberToString(ToChromeTimestampFromTime(from_time)),
       base::NumberToString(ToChromeTimestampFromTime(to_time))},
      nullptr);
  BindColumnTypes(&*mojom_statement);
  mojom_transaction->statements.push_back(std::move(mojom_statement));

  RunDBTransaction(std::move(mojom_transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void Transactions::Reconcile(const PaymentTokenList& payment_tokens,
                             ResultCallback callback) const {
  std::vector<std::string> transaction_ids;
  for (const auto& payment_token : payment_tokens) {
    transaction_ids.push_back(payment_token.transaction_id);
  }

  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type = mojom::DBStatementInfo::OperationType::kRun;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
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
      {GetTableName(),
       base::NumberToString(ToChromeTimestampFromTime(base::Time::Now())),
       BuildBindColumnPlaceholder(
           /*column_count=*/transaction_ids.size()),
       BuildBindColumnPlaceholder(/*column_count=*/1)},
      nullptr);

  int index = 0;
  for (const auto& transaction_id : transaction_ids) {
    BindColumnString(&*mojom_statement, index, transaction_id);
    ++index;
  }

  mojom_transaction->statements.push_back(std::move(mojom_statement));

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void Transactions::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  Execute(&*mojom_transaction, R"(
            DELETE FROM
              $1
            WHERE
              reconciled_at != 0
            AND DATETIME(
                (created_at / 1000000) - 11644473600,
                'unixepoch'
            ) <= DATETIME(
              ($2 / 1000000) - 11644473600,
              'unixepoch',
              '-3 months'
            );)",
          {GetTableName(),
           base::NumberToString(ToChromeTimestampFromTime(base::Time::Now()))});

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

std::string Transactions::GetTableName() const {
  return kTableName;
}

void Transactions::Create(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  Execute(mojom_transaction, R"(
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

  // Optimize database query for `GetForDateRange` from schema 35.
  CreateTableIndex(mojom_transaction, GetTableName(),
                   /*columns=*/{"created_at"});

  // Optimize database query for `Reconcile` from schema 43.
  CreateTableIndex(mojom_transaction, GetTableName(),
                   /*columns=*/{"reconciled_at"});
  CreateTableIndex(mojom_transaction, GetTableName(), /*columns=*/{"id"});
  CreateTableIndex(mojom_transaction, GetTableName(),
                   /*columns=*/{"creative_instance_id"});
}

void Transactions::Migrate(mojom::DBTransactionInfo* mojom_transaction,
                           const int to_version) {
  CHECK(mojom_transaction);

  switch (to_version) {
    case 18: {
      MigrateToV18(mojom_transaction);
      break;
    }

    case 26: {
      MigrateToV26(mojom_transaction);
      break;
    }

    case 29: {
      MigrateToV29(mojom_transaction);
      break;
    }

    case 32: {
      MigrateToV32(mojom_transaction);
      break;
    }

    case 35: {
      MigrateToV35(mojom_transaction);
      break;
    }

    case 39: {
      MigrateToV39(mojom_transaction);
      break;
    }

    case 40: {
      MigrateToV40(mojom_transaction);
      break;
    }

    case 43: {
      MigrateToV43(mojom_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void Transactions::Insert(mojom::DBTransactionInfo* mojom_transaction,
                          const TransactionList& transactions) {
  CHECK(mojom_transaction);

  if (transactions.empty()) {
    return;
  }

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type = mojom::DBStatementInfo::OperationType::kRun;
  mojom_statement->sql = BuildInsertSql(&*mojom_statement, transactions);
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

std::string Transactions::BuildInsertSql(
    mojom::DBStatementInfo* mojom_statement,
    const TransactionList& transactions) const {
  CHECK(mojom_statement);
  CHECK(!transactions.empty());

  const size_t row_count = BindColumns(mojom_statement, transactions);

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
