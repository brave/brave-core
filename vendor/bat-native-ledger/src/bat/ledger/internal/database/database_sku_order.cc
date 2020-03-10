/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
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

}  // namespace braveledger_database
