/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_sku_order_items.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char kTableName[] = "sku_order_items";

}  // namespace

DatabaseSKUOrderItems::DatabaseSKUOrderItems(bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseSKUOrderItems::~DatabaseSKUOrderItems() = default;

bool DatabaseSKUOrderItems::CreateTableV19(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "order_item_id TEXT NOT NULL,"
      "order_id TEXT NOT NULL,"
      "sku TEXT,"
      "quantity INTEGER,"
      "price DOUBLE,"
      "name TEXT,"
      "description TEXT,"
      "type INTEGER,"
      "expires_at TIMESTAMP,"
      "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
      "CONSTRAINT %s_unique "
      "    UNIQUE (order_item_id, order_id)"
      ")",
      kTableName,
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseSKUOrderItems::CreateIndexV19(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  bool success = this->InsertIndex(transaction, kTableName, "order_id");

  if (!success) {
    return false;
  }

  return this->InsertIndex(transaction, kTableName, "order_item_id");
}

bool DatabaseSKUOrderItems::Migrate(
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

bool DatabaseSKUOrderItems::MigrateToV19(ledger::DBTransaction* transaction) {
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

void DatabaseSKUOrderItems::InsertOrUpdateList(
    ledger::DBTransaction* transaction,
    ledger::SKUOrderItemList list) {
  DCHECK(transaction);
  if (list.empty()) {
    return;
  }

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(order_item_id, order_id, sku, quantity, price, name, description, "
      "type, expires_at) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
      kTableName);

  for (auto& item : list) {
    auto command = ledger::DBCommand::New();
    command->type = ledger::DBCommand::Type::RUN;
    command->command = query;

    BindString(command.get(), 0, item->order_item_id);
    BindString(command.get(), 1, item->order_id);
    BindString(command.get(), 2, item->sku);
    BindInt(command.get(), 3, item->quantity);
    BindDouble(command.get(), 4, item->price);
    BindString(command.get(), 5, item->name);
    BindString(command.get(), 6, item->description);
    BindInt(command.get(), 7, static_cast<int>(item->type));
    BindInt64(command.get(), 8, item->expires_at);

    transaction->commands.push_back(std::move(command));
  }
}

void DatabaseSKUOrderItems::GetRecordsByOrderId(
    const std::string& order_id,
    GetSKUOrderItemsCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT order_item_id, order_id, sku, quantity, price, name, "
      "description, type, expires_at FROM %s WHERE order_id = ?",
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
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseSKUOrderItems::OnGetRecordsByOrderId,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseSKUOrderItems::OnGetRecordsByOrderId(
    ledger::DBCommandResponsePtr response,
    GetSKUOrderItemsCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback({});
    return;
  }

  ledger::SKUOrderItemList list;
  ledger::SKUOrderItemPtr info = nullptr;
  for (auto const& record : response->result->get_records()) {
    auto* record_pointer = record.get();
    info = ledger::SKUOrderItem::New();

    info->order_item_id = GetStringColumn(record_pointer, 0);
    info->order_id = GetStringColumn(record_pointer, 1);
    info->sku = GetStringColumn(record_pointer, 2);
    info->quantity = GetIntColumn(record_pointer, 3);
    info->price = GetDoubleColumn(record_pointer, 4);
    info->name = GetStringColumn(record_pointer, 5);
    info->description = GetStringColumn(record_pointer, 6);
    info->type =
        static_cast<ledger::SKUOrderItemType>(GetIntColumn(record_pointer, 7));
    info->expires_at = GetInt64Column(record_pointer, 8);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

}  // namespace braveledger_database
