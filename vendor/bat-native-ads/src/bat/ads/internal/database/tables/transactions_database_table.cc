/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/transactions_database_table.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_constants.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_formatting_util.h"

namespace ads {
namespace database {
namespace table {

namespace {

const char kTableName[] = "transactions";

int BindParameters(mojom::DBCommand* command,
                   const TransactionList& transactions) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& transaction : transactions) {
    BindString(command, index++, transaction.id);
    BindDouble(command, index++, transaction.created_at);
    BindString(command, index++, transaction.creative_instance_id);
    BindDouble(command, index++, transaction.value);
    BindString(command, index++, std::string(transaction.ad_type));
    BindString(command, index++, std::string(transaction.confirmation_type));
    BindDouble(command, index++, transaction.reconciled_at);

    count++;
  }

  return count;
}

TransactionInfo GetFromRecord(mojom::DBRecord* record) {
  DCHECK(record);

  TransactionInfo transaction;

  transaction.id = ColumnString(record, 0);
  transaction.created_at = ColumnDouble(record, 1);
  transaction.creative_instance_id = ColumnString(record, 2);
  transaction.value = ColumnDouble(record, 3);
  transaction.ad_type = AdType(ColumnString(record, 4));
  transaction.confirmation_type = ConfirmationType(ColumnString(record, 5));
  transaction.reconciled_at = ColumnDouble(record, 6);

  return transaction;
}

}  // namespace

Transactions::Transactions() = default;

Transactions::~Transactions() = default;

void Transactions::Save(const TransactionList& transactions,
                        ResultCallback callback) {
  if (transactions.empty()) {
    callback(/* success */ true);
    return;
  }

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  InsertOrUpdate(transaction.get(), transactions);

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void Transactions::GetAll(GetTransactionsCallback callback) {
  const std::string query = base::StringPrintf(
      "SELECT "
      "id, "
      "created_at, "
      "creative_instance_id, "
      "value, "
      "ad_type, "
      "confirmation_type, "
      "reconciled_at "
      "FROM %s",
      GetTableName().c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // id
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // created_at
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_instance_id
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // ad_type
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // confirmation_type
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE   // reconciled_at
  };

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction), std::bind(&Transactions::OnGetTransactions, this,
                                        std::placeholders::_1, callback));
}

void Transactions::GetForDateRange(const base::Time& from_time,
                                   const base::Time& to_time,
                                   GetTransactionsCallback callback) {
  const std::string query = base::StringPrintf(
      "SELECT "
      "id, "
      "created_at, "
      "creative_instance_id, "
      "value, "
      "ad_type, "
      "confirmation_type, "
      "reconciled_at "
      "FROM %s "
      "WHERE created_at BETWEEN %f and %f ",
      GetTableName().c_str(), from_time.ToDoubleT(), to_time.ToDoubleT());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // id
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // created_at
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_instance_id
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // ad_type
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // confirmation_type
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE   // reconciled_at
  };

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction), std::bind(&Transactions::OnGetTransactions, this,
                                        std::placeholders::_1, callback));
}

void Transactions::Update(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens,
    ResultCallback callback) {
  std::vector<std::string> transaction_ids;
  for (const auto& unblinded_payment_token : unblinded_payment_tokens) {
    transaction_ids.push_back(unblinded_payment_token.transaction_id);
  }
  transaction_ids.push_back(rewards::kMigrationUnreconciledTransactionId);

  const std::string query = base::StringPrintf(
      "UPDATE %s "
      "SET reconciled_at = %s "
      "WHERE reconciled_at == 0 "
      "AND id IN %s",
      GetTableName().c_str(), TimeAsTimestampString(base::Time::Now()).c_str(),
      BuildBindingParameterPlaceholder(transaction_ids.size()).c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  int index = 0;
  for (const auto& transaction_id : transaction_ids) {
    BindString(command.get(), index, transaction_id);
    index++;
  }

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void Transactions::Delete(ResultCallback callback) {
  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  util::Delete(transaction.get(), GetTableName());

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

std::string Transactions::GetTableName() const {
  return kTableName;
}

void Transactions::Migrate(mojom::DBTransaction* transaction,
                           const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 18: {
      MigrateToV18(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void Transactions::InsertOrUpdate(mojom::DBTransaction* transaction,
                                  const TransactionList& transactions) {
  DCHECK(transaction);

  if (transactions.empty()) {
    return;
  }

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), transactions);

  transaction->commands.push_back(std::move(command));
}

std::string Transactions::BuildInsertOrUpdateQuery(
    mojom::DBCommand* command,
    const TransactionList& transactions) {
  DCHECK(command);

  const int count = BindParameters(command, transactions);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(id, "
      "created_at, "
      "creative_instance_id, "
      "value, "
      "ad_type, "
      "confirmation_type, "
      "reconciled_at) VALUES %s",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(7, count).c_str());
}

void Transactions::OnGetTransactions(mojom::DBCommandResponsePtr response,
                                     GetTransactionsCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get transactions");
    callback(/* success */ false, {});
    return;
  }

  TransactionList transactions;

  for (const auto& record : response->result->get_records()) {
    TransactionInfo info = GetFromRecord(record.get());
    transactions.push_back(info);
  }

  callback(/* success */ true, transactions);
}

void Transactions::MigrateToV18(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE transactions "
      "(id TEXT NOT NULL PRIMARY KEY UNIQUE ON CONFLICT REPLACE, "
      "created_at TIMESTAMP NOT NULL, "
      "creative_instance_id TEXT, "
      "value DOUBLE NOT NULL, "
      "ad_type TEXT NOT NULL, "
      "confirmation_type TEXT NOT NULL, "
      "reconciled_at TIMESTAMP)");

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  util::CreateIndex(transaction, "transactions", "id");
}

}  // namespace table
}  // namespace database
}  // namespace ads
