/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/bind_util.h"
#include "bat/ledger/internal/database/database_sku_order.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char kTableName[] = "sku_order";

}  // namespace

DatabaseSKUOrder::DatabaseSKUOrder(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger),
    items_(std::make_unique<DatabaseSKUOrderItems>(ledger)) {
}

DatabaseSKUOrder::~DatabaseSKUOrder() = default;

bool DatabaseSKUOrder::CreateTableV19(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "order_id TEXT NOT NULL,"
      "total_amount DOUBLE,"
      "merchant_id TEXT,"
      "location TEXT,"
      "status INTEGER NOT NULL DEFAULT 0,"
      "contribution_id TEXT,"
      "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
      "PRIMARY KEY (order_id)"
      ")",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseSKUOrder::Migrate(
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

bool DatabaseSKUOrder::MigrateToV19(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, kTableName)) {
    return false;
  }

  if (!CreateTableV19(transaction)) {
    return false;
  }

  return items_->Migrate(transaction, 19);
}

void DatabaseSKUOrder::InsertOrUpdate(
    ledger::SKUOrderPtr order,
    ledger::ResultCallback callback) {
  if (!order) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(order_id, total_amount, merchant_id, location, status, "
      "contribution_id) "
      "VALUES (?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, order->order_id);
  BindDouble(command.get(), 1, order->total_amount);
  BindString(command.get(), 2, order->merchant_id);
  BindString(command.get(), 3, order->location);
  BindInt(command.get(), 4, static_cast<int>(order->status));
  BindString(command.get(), 5, order->contribution_id);

  transaction->commands.push_back(std::move(command));

  items_->InsertOrUpdateList(transaction.get(), std::move(order->items));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseSKUOrder::UpdateStatus(
    const std::string& order_id,
    const ledger::SKUOrderStatus status,
    ledger::ResultCallback callback) {
  if (order_id.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ? WHERE order_id = ?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(status));
  BindString(command.get(), 1, order_id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseSKUOrder::GetRecord(
    const std::string& order_id,
    ledger::GetSKUOrderCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT order_id, total_amount, merchant_id, location, status, "
      "created_at FROM %s WHERE order_id = ?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, order_id);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabaseSKUOrder::OnGetRecord,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseSKUOrder::OnGetRecord(
    ledger::DBCommandResponsePtr response,
    ledger::GetSKUOrderCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback({});
    return;
  }

  if (response->result->get_records().size() != 1) {
    callback({});
    return;
  }

  auto* record = response->result->get_records()[0].get();
  auto info = ledger::SKUOrder::New();
  info->order_id = GetStringColumn(record, 0);
  info->total_amount = GetDoubleColumn(record, 1);
  info->merchant_id = GetStringColumn(record, 2);
  info->location = GetStringColumn(record, 3);
  info->status = static_cast<ledger::SKUOrderStatus>(GetIntColumn(record, 4));
  info->created_at = GetInt64Column(record, 5);

  auto items_callback = std::bind(&DatabaseSKUOrder::OnGetRecordItems,
      this,
      _1,
      braveledger_bind_util::FromSKUOrderToString(info->Clone()),
      callback);
  items_->GetRecordsByOrderId(info->order_id, items_callback);
}

void DatabaseSKUOrder::OnGetRecordItems(
    ledger::SKUOrderItemList list,
    const std::string& order_string,
    ledger::GetSKUOrderCallback callback) {
  auto order = braveledger_bind_util::FromStringToSKUOrder(order_string);
  if (!order) {
    callback({});
    return;
  }

  order->items = std::move(list);
  callback(std::move(order));
}

}  // namespace braveledger_database
