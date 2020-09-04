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

namespace ledger {
namespace database {

namespace {

const char kTableName[] = "sku_order_items";

}  // namespace

DatabaseSKUOrderItems::DatabaseSKUOrderItems(LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseSKUOrderItems::~DatabaseSKUOrderItems() = default;

void DatabaseSKUOrderItems::InsertOrUpdateList(
    type::DBTransaction* transaction,
    type::SKUOrderItemList list) {
  DCHECK(transaction);
  if (list.empty()) {
    BLOG(1, "List is empty");
    return;
  }

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(order_item_id, order_id, sku, quantity, price, name, description, "
      "type, expires_at) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
      kTableName);

  for (auto& item : list) {
    auto command = type::DBCommand::New();
    command->type = type::DBCommand::Type::RUN;
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
  if (order_id.empty()) {
    BLOG(1, "Order id is empty");
    callback({});
    return;
  }

  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT order_item_id, order_id, sku, quantity, price, name, "
      "description, type, expires_at FROM %s WHERE order_id = ?",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, order_id);

  command->record_bindings = {
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::INT_TYPE,
      type::DBCommand::RecordBindingType::DOUBLE_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::INT_TYPE,
      type::DBCommand::RecordBindingType::INT64_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseSKUOrderItems::OnGetRecordsByOrderId,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseSKUOrderItems::OnGetRecordsByOrderId(
    type::DBCommandResponsePtr response,
    GetSKUOrderItemsCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  type::SKUOrderItemList list;
  type::SKUOrderItemPtr info = nullptr;
  for (auto const& record : response->result->get_records()) {
    auto* record_pointer = record.get();
    info = type::SKUOrderItem::New();

    info->order_item_id = GetStringColumn(record_pointer, 0);
    info->order_id = GetStringColumn(record_pointer, 1);
    info->sku = GetStringColumn(record_pointer, 2);
    info->quantity = GetIntColumn(record_pointer, 3);
    info->price = GetDoubleColumn(record_pointer, 4);
    info->name = GetStringColumn(record_pointer, 5);
    info->description = GetStringColumn(record_pointer, 6);
    info->type =
        static_cast<type::SKUOrderItemType>(GetIntColumn(record_pointer, 7));
    info->expires_at = GetInt64Column(record_pointer, 8);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

}  // namespace database
}  // namespace ledger
