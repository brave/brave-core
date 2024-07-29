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
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "transactions";

void BindRecords(mojom::DBCommandInfo* const command) {
  CHECK(command);

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // id
      mojom::DBCommandInfo::RecordBindingType::INT64_TYPE,   // created_at
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // ad_type
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,                                     // confirmation_type
      mojom::DBCommandInfo::RecordBindingType::INT64_TYPE  // reconciled_at
  };
}

size_t BindParameters(mojom::DBCommandInfo* command,
                      const TransactionList& transactions) {
  CHECK(command);

  size_t count = 0;

  int index = 0;
  for (const auto& transaction : transactions) {
    if (!transaction.IsValid()) {
      base::debug::DumpWithoutCrashing();
      BLOG(0, "Invalid transaction");
      continue;
    }

    BindString(command, index++, transaction.id);
    BindInt64(command, index++,
              ToChromeTimestampFromTime(
                  transaction.created_at.value_or(base::Time())));
    BindString(command, index++, transaction.creative_instance_id);
    BindDouble(command, index++, transaction.value);
    BindString(command, index++, transaction.segment);
    BindString(command, index++, ToString(transaction.ad_type));
    BindString(command, index++, ToString(transaction.confirmation_type));
    BindInt64(command, index++,
              ToChromeTimestampFromTime(
                  transaction.reconciled_at.value_or(base::Time())));

    ++count;
  }

  return count;
}

TransactionInfo GetFromRecord(mojom::DBRecordInfo* const record) {
  CHECK(record);

  TransactionInfo transaction;

  transaction.id = ColumnString(record, 0);
  const base::Time created_at =
      ToTimeFromChromeTimestamp(ColumnInt64(record, 1));
  if (!created_at.is_null()) {
    transaction.created_at = created_at;
  }
  transaction.creative_instance_id = ColumnString(record, 2);
  transaction.value = ColumnDouble(record, 3);
  transaction.segment = ColumnString(record, 4);
  transaction.ad_type = ToAdType(ColumnString(record, 5));
  transaction.confirmation_type = ToConfirmationType(ColumnString(record, 6));
  const base::Time reconciled_at =
      ToTimeFromChromeTimestamp(ColumnInt64(record, 7));
  if (!reconciled_at.is_null()) {
    transaction.reconciled_at = reconciled_at;
  }

  return transaction;
}

void GetCallback(GetTransactionsCallback callback,
                 mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get transactions");

    return std::move(callback).Run(/*success=*/false, /*transactions=*/{});
  }

  CHECK(command_response->result);

  TransactionList transactions;

  for (const auto& record : command_response->result->get_records()) {
    const TransactionInfo transaction = GetFromRecord(&*record);
    if (!transaction.IsValid()) {
      base::debug::DumpWithoutCrashing();
      BLOG(0, "Invalid transaction");
      continue;
    }

    transactions.push_back(transaction);
  }

  std::move(callback).Run(/*success=*/true, transactions);
}

void MigrateToV18(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          CREATE TABLE transactions (
            id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
            created_at TIMESTAMP NOT NULL,
            creative_instance_id TEXT,
            value DOUBLE NOT NULL,
            ad_type TEXT NOT NULL,
            confirmation_type TEXT NOT NULL,
            reconciled_at TIMESTAMP
          );)";
  transaction->commands.push_back(std::move(command));
}

void MigrateToV26(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Create a temporary table:
  //   - with a new `segment` column.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          CREATE TABLE transactions_temp (
            id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
            created_at TIMESTAMP NOT NULL,
            creative_instance_id TEXT,
            value DOUBLE NOT NULL,
            segment TEXT,
            ad_type TEXT NOT NULL,
            confirmation_type TEXT NOT NULL,
            reconciled_at TIMESTAMP
          );)";
  transaction->commands.push_back(std::move(command));

  // Copy legacy columns to the temporary table, drop the legacy table, rename
  // the temporary table and create an index.
  const std::vector<std::string> columns = {
      "id",      "created_at",        "creative_instance_id", "value",
      "ad_type", "confirmation_type", "reconciled_at"};

  CopyTableColumns(transaction, "transactions", "transactions_temp", columns,
                   /*should_drop=*/true);

  RenameTable(transaction, "transactions_temp", "transactions");
}

void MigrateToV29(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  {
    // Migrate `created_at` column from an Epoch timestamp to a WebKit/Chrome
    // timestamp.
    mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
    command->type = mojom::DBCommandInfo::Type::EXECUTE;
    command->sql =
        R"(
            UPDATE
              transactions
            SET
              created_at = (
                CAST(created_at AS INT64) + 11644473600
              ) * 1000000;)";
    transaction->commands.push_back(std::move(command));
  }

  {
    // Migrate `reconciled_at` column from an Epoch timestamp to a WebKit/Chrome
    // timestamp.
    mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
    command->type = mojom::DBCommandInfo::Type::EXECUTE;
    command->sql =
        R"(
            UPDATE
              transactions
            SET
              reconciled_at = (
                CAST(reconciled_at AS INT64) + 11644473600
              ) * 1000000
            WHERE
              reconciled_at != 0;)";
    transaction->commands.push_back(std::move(command));
  }
}

void MigrateToV32(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Migrate `confirmation_type` from 'saved' to 'bookmark'.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          UPDATE
            transactions
          SET
            confirmation_type = 'bookmark'
          WHERE
            confirmation_type == 'saved';)";
  transaction->commands.push_back(std::move(command));
}

void MigrateToV35(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Optimize database query for `GetForDateRange`.
  CreateTableIndex(transaction, /*table_name=*/"transactions",
                   /*columns=*/{"created_at"});
}

void MigrateToV39(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Delete legacy transactions with an undefined `created_at` timestamp.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          DELETE FROM
            transactions
          WHERE
            created_at == 0;)";
  transaction->commands.push_back(std::move(command));
}

void MigrateToV40(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  {
    // Delete legacy transactions with an undefined `creative_instance_id`,
    // `segment` or `ad_type`.
    mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
    command->type = mojom::DBCommandInfo::Type::EXECUTE;
    command->sql =
        R"(
          DELETE FROM
            transactions
          WHERE
            COALESCE(creative_instance_id, '') = ''
            OR COALESCE(segment, '') = ''
            OR ad_type = '';)";
    transaction->commands.push_back(std::move(command));
  }

  {
    // Create a temporary table:
    //   - with a new `creative_instance_id` column constraint.
    //   - with a new `segment` column constraint.
    //   - with a new `reconciled_at` default value.
    mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
    command->type = mojom::DBCommandInfo::Type::EXECUTE;
    command->sql =
        R"(
          CREATE TABLE transactions_temp (
            id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
            created_at TIMESTAMP NOT NULL,
            creative_instance_id TEXT NOT NULL,
            value DOUBLE NOT NULL,
            segment TEXT NOT NULL,
            ad_type TEXT NOT NULL,
            confirmation_type TEXT NOT NULL,
            reconciled_at TIMESTAMP DEFAULT 0
          );)";
    transaction->commands.push_back(std::move(command));

    // Copy legacy columns to the temporary table, drop the legacy table,
    // rename the temporary table and create an index.
    const std::vector<std::string> columns = {
        "id",      "created_at", "creative_instance_id", "value",
        "segment", "ad_type",    "confirmation_type",    "reconciled_at"};

    CopyTableColumns(transaction, "transactions", "transactions_temp", columns,
                     /*should_drop=*/true);

    RenameTable(transaction, "transactions_temp", "transactions");
  }

  // Optimize database query for `GetForDateRange`.
  CreateTableIndex(transaction, /*table_name=*/"transactions",
                   /*columns=*/{"created_at"});
}

}  // namespace

void Transactions::Save(const TransactionList& transactions,
                        ResultCallback callback) {
  if (transactions.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  InsertOrUpdate(&*transaction, transactions);

  RunTransaction(std::move(transaction), std::move(callback));
}

void Transactions::GetForDateRange(const base::Time from_time,
                                   const base::Time to_time,
                                   GetTransactionsCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
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
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void Transactions::Reconcile(const PaymentTokenList& payment_tokens,
                             ResultCallback callback) const {
  std::vector<std::string> transaction_ids;
  for (const auto& payment_token : payment_tokens) {
    transaction_ids.push_back(payment_token.transaction_id);
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = base::ReplaceStringPlaceholders(
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
       BuildBindingParameterPlaceholder(
           /*parameters_count=*/transaction_ids.size()),
       BuildBindingParameterPlaceholder(/*parameters_count=*/1)},
      nullptr);

  int index = 0;
  for (const auto& transaction_id : transaction_ids) {
    BindString(&*command, index, transaction_id);
    ++index;
  }

  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

void Transactions::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(&*transaction, GetTableName());

  RunTransaction(std::move(transaction), std::move(callback));
}

std::string Transactions::GetTableName() const {
  return kTableName;
}

void Transactions::Create(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          CREATE TABLE transactions (
            id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
            created_at TIMESTAMP NOT NULL,
            creative_instance_id TEXT NOT NULL,
            value DOUBLE NOT NULL,
            segment TEXT NOT NULL,
            ad_type TEXT NOT NULL,
            confirmation_type TEXT NOT NULL,
            reconciled_at TIMESTAMP DEFAULT 0
          );)";
  transaction->commands.push_back(std::move(command));

  // Optimize database query for `GetForDateRange`.
  CreateTableIndex(transaction, GetTableName(), /*columns=*/{"created_at"});
}

void Transactions::Migrate(mojom::DBTransactionInfo* transaction,
                           const int to_version) {
  CHECK(transaction);

  switch (to_version) {
    case 18: {
      MigrateToV18(transaction);
      break;
    }

    case 26: {
      MigrateToV26(transaction);
      break;
    }

    case 29: {
      MigrateToV29(transaction);
      break;
    }

    case 32: {
      MigrateToV32(transaction);
      break;
    }

    case 35: {
      MigrateToV35(transaction);
      break;
    }

    case 39: {
      MigrateToV39(transaction);
      break;
    }

    case 40: {
      MigrateToV40(transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void Transactions::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                                  const TransactionList& transactions) {
  CHECK(transaction);

  if (transactions.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, transactions);
  transaction->commands.push_back(std::move(command));
}

std::string Transactions::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const TransactionList& transactions) const {
  CHECK(command);

  const size_t binded_parameters_count = BindParameters(command, transactions);

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
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count=*/8, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
