/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_sku_transaction.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char kTableName[] = "sku_transaction";

}  // namespace

DatabaseSKUTransaction::DatabaseSKUTransaction(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseSKUTransaction::~DatabaseSKUTransaction() = default;

bool DatabaseSKUTransaction::CreateTableV19(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "transaction_id TEXT NOT NULL,"
      "order_id TEXT NOT NULL,"
      "external_transaction_id TEXT NOT NULL,"
      "type INTEGER NOT NULL,"
      "amount DOUBLE NOT NULL,"
      "status INTEGER NOT NULL,"
      "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
      "PRIMARY KEY (transaction_id)"
      ")",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseSKUTransaction::CreateIndexV19(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "order_id");
}

bool DatabaseSKUTransaction::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 19: {
      return MigrateToV19(transaction);
    }
    default: {
      return true;
    }
  }
}

bool DatabaseSKUTransaction::MigrateToV19(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, kTableName)) {
    return false;
  }

  if (!CreateTableV19(transaction)) {
    return false;
  }

  if (!CreateIndexV19(transaction)) {
    return false;
  }

  return true;
}

void DatabaseSKUTransaction::InsertOrUpdate(
    ledger::SKUTransactionPtr transaction,
    ledger::ResultCallback callback) {
  if (!transaction) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto db_transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(transaction_id, order_id, external_transaction_id, type, amount, "
      "status) "
      "VALUES (?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, transaction->transaction_id);
  BindString(command.get(), 1, transaction->order_id);
  BindString(command.get(), 2, transaction->external_transaction_id);
  BindInt(command.get(), 3, static_cast<int>(transaction->type));
  BindDouble(command.get(), 4, transaction->amount);
  BindInt(command.get(), 5, static_cast<int>(transaction->status));

  db_transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(db_transaction), transaction_callback);
}

void DatabaseSKUTransaction::SaveExternalTransaction(
    const std::string& transaction_id,
    const std::string& external_transaction_id,
    ledger::ResultCallback callback) {
  if (transaction_id.empty() || external_transaction_id.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET "
      "external_transaction_id = ?, status = ? WHERE transaction_id = ?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, external_transaction_id);
  BindInt(command.get(), 1, static_cast<int>(
      ledger::SKUTransactionStatus::COMPLETED));
  BindString(command.get(), 2, transaction_id);

  auto transaction = ledger::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseSKUTransaction::GetRecordByOrderId(
    const std::string& order_id,
    ledger::GetSKUTransactionCallback callback) {
  if (order_id.empty()) {
    callback(nullptr);
    return;
  }
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT transaction_id, order_id, external_transaction_id, amount, type, "
    "status FROM %s WHERE order_id = ?",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, order_id);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabaseSKUTransaction::OnGetRecord,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseSKUTransaction::OnGetRecord(
    ledger::DBCommandResponsePtr response,
    ledger::GetSKUTransactionCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback(nullptr);
    return;
  }

  if (response->result->get_records().size() != 1) {
    callback(nullptr);
    return;
  }

  auto* record = response->result->get_records()[0].get();

  auto info = ledger::SKUTransaction::New();
  info->transaction_id = GetStringColumn(record, 0);
  info->order_id = GetStringColumn(record, 1);
  info->external_transaction_id = GetStringColumn(record, 2);
  info->amount = GetDoubleColumn(record, 3);
  info->type =
      static_cast<ledger::SKUTransactionType>(GetIntColumn(record, 4));
  info->status =
      static_cast<ledger::SKUTransactionStatus>(GetIntColumn(record, 5));

  callback(std::move(info));
}

}  // namespace braveledger_database
