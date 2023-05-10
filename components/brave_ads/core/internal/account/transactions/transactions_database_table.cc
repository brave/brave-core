/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_constants.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "transactions";

void BindRecords(mojom::DBCommandInfo* command) {
  CHECK(command);

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // id
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // created_at
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // ad_type
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,                                      // confirmation_type
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE  // reconciled_at
  };
}

size_t BindParameters(mojom::DBCommandInfo* command,
                      const TransactionList& transactions) {
  CHECK(command);

  size_t count = 0;

  int index = 0;
  for (const auto& transaction : transactions) {
    BindString(command, index++, transaction.id);
    BindDouble(command, index++, transaction.created_at.ToDoubleT());
    BindString(command, index++, transaction.creative_instance_id);
    BindDouble(command, index++, transaction.value);
    BindString(command, index++, transaction.segment);
    BindString(command, index++, transaction.ad_type.ToString());
    BindString(command, index++, transaction.confirmation_type.ToString());
    BindDouble(command, index++, transaction.reconciled_at.ToDoubleT());

    count++;
  }

  return count;
}

TransactionInfo GetFromRecord(mojom::DBRecordInfo* record) {
  CHECK(record);

  TransactionInfo transaction;

  transaction.id = ColumnString(record, 0);
  transaction.created_at = base::Time::FromDoubleT(ColumnDouble(record, 1));
  transaction.creative_instance_id = ColumnString(record, 2);
  transaction.value = ColumnDouble(record, 3);
  transaction.segment = ColumnString(record, 4);
  transaction.ad_type = AdType(ColumnString(record, 5));
  transaction.confirmation_type = ConfirmationType(ColumnString(record, 6));
  transaction.reconciled_at = base::Time::FromDoubleT(ColumnDouble(record, 7));

  return transaction;
}

void OnGetTransactions(GetTransactionsCallback callback,
                       mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get transactions");
    return std::move(callback).Run(/*success*/ false, /*transactions*/ {});
  }

  TransactionList transactions;

  for (const auto& record : command_response->result->get_records()) {
    const TransactionInfo transaction = GetFromRecord(&*record);
    transactions.push_back(transaction);
  }

  std::move(callback).Run(/*success*/ true, transactions);
}

void MigrateToV18(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE IF NOT EXISTS transactions (id TEXT NOT NULL PRIMARY KEY "
      "UNIQUE ON CONFLICT REPLACE, created_at TIMESTAMP NOT NULL, "
      "creative_instance_id TEXT, value DOUBLE NOT NULL, ad_type TEXT NOT "
      "NULL, confirmation_type TEXT NOT NULL, reconciled_at TIMESTAMP);";
  transaction->commands.push_back(std::move(command));

  CreateTableIndex(transaction, "transactions", "id");
}

void MigrateToV26(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  // Create a temporary table with new |segment| column.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE transactions_temp (id TEXT NOT NULL PRIMARY KEY UNIQUE ON "
      "CONFLICT REPLACE, created_at TIMESTAMP NOT NULL, creative_instance_id "
      "TEXT, value DOUBLE NOT NULL, segment TEXT, ad_type TEXT NOT NULL, "
      "confirmation_type TEXT NOT NULL, reconciled_at TIMESTAMP);";
  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table.
  const std::vector<std::string> columns = {
      "id",      "created_at",        "creative_instance_id", "value",
      "ad_type", "confirmation_type", "reconciled_at"};

  CopyTableColumns(transaction, "transactions", "transactions_temp", columns,
                   /*should_drop*/ true);

  // Rename temporary table.
  RenameTable(transaction, "transactions_temp", "transactions");

  CreateTableIndex(transaction, "transactions", "id");
}

}  // namespace

void Transactions::Save(const TransactionList& transactions,
                        ResultCallback callback) {
  if (transactions.empty()) {
    return std::move(callback).Run(/*success*/ true);
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  InsertOrUpdate(&*transaction, transactions);

  RunTransaction(std::move(transaction), std::move(callback));
}

void Transactions::GetAll(GetTransactionsCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      "SELECT id, created_at, creative_instance_id, value, segment, ad_type, "
      "confirmation_type, reconciled_at FROM $1;",
      {GetTableName()}, nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnGetTransactions, std::move(callback)));
}

void Transactions::GetForDateRange(const base::Time from_time,
                                   const base::Time to_time,
                                   GetTransactionsCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      "SELECT id, created_at, creative_instance_id, value, segment, ad_type, "
      "confirmation_type, reconciled_at FROM $1 WHERE created_at BETWEEN $2 "
      "AND $3;",
      {GetTableName(), TimeAsTimestampString(from_time),
       TimeAsTimestampString(to_time)},
      nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnGetTransactions, std::move(callback)));
}

void Transactions::Update(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens,
    ResultCallback callback) const {
  std::vector<std::string> transaction_ids;
  for (const auto& unblinded_payment_token : unblinded_payment_tokens) {
    transaction_ids.push_back(unblinded_payment_token.transaction_id);
  }
  transaction_ids.emplace_back(rewards::kMigrationUnreconciledTransactionId);

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = base::ReplaceStringPlaceholders(
      "UPDATE $1 SET reconciled_at = $2 WHERE reconciled_at == 0 AND (id IN $3 "
      "OR creative_instance_id IN $4);",
      {GetTableName(), TimeAsTimestampString(base::Time::Now()),
       BuildBindingParameterPlaceholder(transaction_ids.size()),
       BuildBindingParameterPlaceholder(1)},
      nullptr);

  int index = 0;
  for (const auto& transaction_id : transaction_ids) {
    BindString(&*command, index, transaction_id);
    index++;
  }

  BindString(&*command, index, rewards::kMigrationUnreconciledTransactionId);

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

void Transactions::Create(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE transactions (id TEXT NOT NULL PRIMARY KEY "
      "UNIQUE ON CONFLICT REPLACE, created_at TIMESTAMP NOT NULL, "
      "creative_instance_id TEXT, value DOUBLE NOT NULL, segment TEXT, ad_type "
      "TEXT NOT NULL, confirmation_type TEXT NOT NULL, reconciled_at "
      "TIMESTAMP);";
  transaction->commands.push_back(std::move(command));
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

    default: {
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
      "INSERT OR REPLACE INTO $1 (id, created_at, creative_instance_id, "
      "value, segment, ad_type, confirmation_type, reconciled_at) VALUES $2;",
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count*/ 8, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
