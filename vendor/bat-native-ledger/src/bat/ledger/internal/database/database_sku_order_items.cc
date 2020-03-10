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

}  // namespace braveledger_database
