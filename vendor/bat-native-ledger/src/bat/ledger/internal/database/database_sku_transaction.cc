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

namespace ledger {
namespace database {

namespace {

const char kTableName[] = "sku_transaction";

}  // namespace

DatabaseSKUTransaction::DatabaseSKUTransaction(
    LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseSKUTransaction::~DatabaseSKUTransaction() = default;

void DatabaseSKUTransaction::InsertOrUpdate(
    type::SKUTransactionPtr transaction,
    ledger::ResultCallback callback) {
  if (!transaction) {
    BLOG(1, "Transcation is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto db_transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(transaction_id, order_id, external_transaction_id, type, amount, "
      "status) "
      "VALUES (?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
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

  ledger_->ledger_client()->RunDBTransaction(
      std::move(db_transaction),
      transaction_callback);
}

void DatabaseSKUTransaction::SaveExternalTransaction(
    const std::string& transaction_id,
    const std::string& external_transaction_id,
    ledger::ResultCallback callback) {
  if (transaction_id.empty() || external_transaction_id.empty()) {
    BLOG(1, "Data is empty " <<
        transaction_id << "/" << external_transaction_id);
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET "
      "external_transaction_id = ?, status = ? WHERE transaction_id = ?",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, external_transaction_id);
  BindInt(command.get(), 1, static_cast<int>(
      type::SKUTransactionStatus::COMPLETED));
  BindString(command.get(), 2, transaction_id);

  auto transaction = type::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseSKUTransaction::GetRecordByOrderId(
    const std::string& order_id,
    GetSKUTransactionCallback callback) {
  if (order_id.empty()) {
    BLOG(1, "Order id is empty");
    callback(nullptr);
    return;
  }
  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT transaction_id, order_id, external_transaction_id, amount, type, "
    "status FROM %s WHERE order_id = ?",
    kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, order_id);

  command->record_bindings = {type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE};

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabaseSKUTransaction::OnGetRecord,
      this,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseSKUTransaction::OnGetRecord(
    type::DBCommandResponsePtr response,
    GetSKUTransactionCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(nullptr);
    return;
  }

  if (response->result->get_records().size() != 1) {
    BLOG(1, "Record size is not correct: " <<
        response->result->get_records().size());
    callback(nullptr);
    return;
  }

  auto* record = response->result->get_records()[0].get();

  auto info = type::SKUTransaction::New();
  info->transaction_id = GetStringColumn(record, 0);
  info->order_id = GetStringColumn(record, 1);
  info->external_transaction_id = GetStringColumn(record, 2);
  info->amount = GetDoubleColumn(record, 3);
  info->type =
      static_cast<type::SKUTransactionType>(GetIntColumn(record, 4));
  info->status =
      static_cast<type::SKUTransactionStatus>(GetIntColumn(record, 5));

  callback(std::move(info));
}

}  // namespace database
}  // namespace ledger
