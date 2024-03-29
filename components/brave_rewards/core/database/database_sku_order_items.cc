/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/database/database_sku_order_items.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::database {

namespace {

constexpr char kTableName[] = "sku_order_items";

}  // namespace

DatabaseSKUOrderItems::DatabaseSKUOrderItems(RewardsEngine& engine)
    : DatabaseTable(engine) {}

DatabaseSKUOrderItems::~DatabaseSKUOrderItems() = default;

void DatabaseSKUOrderItems::InsertOrUpdateList(
    mojom::DBTransaction* transaction,
    std::vector<mojom::SKUOrderItemPtr> list) {
  DCHECK(transaction);
  if (list.empty()) {
    engine_->Log(FROM_HERE) << "List is empty";
    return;
  }

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(order_item_id, order_id, sku, quantity, price, name, description, "
      "type, expires_at) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
      kTableName);

  for (auto& item : list) {
    auto command = mojom::DBCommand::New();
    command->type = mojom::DBCommand::Type::kRun;
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
    engine_->Log(FROM_HERE) << "Order id is empty";
    std::move(callback).Run({});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT order_item_id, order_id, sku, quantity, price, name, "
      "description, type, expires_at FROM %s WHERE order_id = ?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRead;
  command->command = query;

  BindString(command.get(), 0, order_id);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kInt,
                              mojom::DBCommand::RecordBindingType::kDouble,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kInt,
                              mojom::DBCommand::RecordBindingType::kInt64};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseSKUOrderItems::OnGetRecordsByOrderId,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseSKUOrderItems::OnGetRecordsByOrderId(
    GetSKUOrderItemsCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::kSuccess) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    std::move(callback).Run({});
    return;
  }

  std::vector<mojom::SKUOrderItemPtr> list;
  mojom::SKUOrderItemPtr info = nullptr;
  for (auto const& record : response->records) {
    auto* record_pointer = record.get();
    info = mojom::SKUOrderItem::New();

    info->order_item_id = GetStringColumn(record_pointer, 0);
    info->order_id = GetStringColumn(record_pointer, 1);
    info->sku = GetStringColumn(record_pointer, 2);
    info->quantity = GetIntColumn(record_pointer, 3);
    info->price = GetDoubleColumn(record_pointer, 4);
    info->name = GetStringColumn(record_pointer, 5);
    info->description = GetStringColumn(record_pointer, 6);
    info->type = SKUOrderItemTypeFromInt(GetIntColumn(record_pointer, 7));
    info->expires_at = GetInt64Column(record_pointer, 8);

    list.push_back(std::move(info));
  }

  std::move(callback).Run(std::move(list));
}

}  // namespace brave_rewards::internal::database
