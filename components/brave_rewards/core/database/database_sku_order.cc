/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/database/database_sku_order.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

using std::placeholders::_1;

namespace brave_rewards::internal {
namespace database {

namespace {

const char kTableName[] = "sku_order";

}  // namespace

DatabaseSKUOrder::DatabaseSKUOrder(RewardsEngineImpl& engine)
    : DatabaseTable(engine), items_(engine) {}

DatabaseSKUOrder::~DatabaseSKUOrder() = default;

void DatabaseSKUOrder::InsertOrUpdate(mojom::SKUOrderPtr order,
                                      LegacyResultCallback callback) {
  if (!order) {
    engine_->Log(FROM_HERE) << "Order is null";
    callback(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(order_id, total_amount, merchant_id, location, status, "
      "contribution_id) "
      "VALUES (?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, order->order_id);
  BindDouble(command.get(), 1, order->total_amount);
  BindString(command.get(), 2, order->merchant_id);
  BindString(command.get(), 3, order->location);
  BindInt(command.get(), 4, static_cast<int>(order->status));
  BindString(command.get(), 5, order->contribution_id);

  transaction->commands.push_back(std::move(command));

  items_.InsertOrUpdateList(transaction.get(), std::move(order->items));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseSKUOrder::UpdateStatus(const std::string& order_id,
                                    mojom::SKUOrderStatus status,
                                    LegacyResultCallback callback) {
  if (order_id.empty()) {
    engine_->Log(FROM_HERE) << "Order id is empty";
    callback(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ? WHERE order_id = ?", kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(status));
  BindString(command.get(), 1, order_id);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseSKUOrder::GetRecord(const std::string& order_id,
                                 GetSKUOrderCallback callback) {
  if (order_id.empty()) {
    engine_->Log(FROM_HERE) << "Order id is empty";
    callback({});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT order_id, total_amount, merchant_id, location, status, "
      "contribution_id, created_at FROM %s WHERE order_id = ?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, order_id);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseSKUOrder::OnGetRecord, base::Unretained(this),
                     std::move(callback)));
}

void DatabaseSKUOrder::OnGetRecord(GetSKUOrderCallback callback,
                                   mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    callback({});
    return;
  }

  if (response->result->get_records().size() != 1) {
    engine_->Log(FROM_HERE) << "Record size is not correct: "
                            << response->result->get_records().size();
    callback({});
    return;
  }

  auto* record = response->result->get_records()[0].get();
  auto info = mojom::SKUOrder::New();
  info->order_id = GetStringColumn(record, 0);
  info->total_amount = GetDoubleColumn(record, 1);
  info->merchant_id = GetStringColumn(record, 2);
  info->location = GetStringColumn(record, 3);
  info->status = static_cast<mojom::SKUOrderStatus>(GetIntColumn(record, 4));
  info->contribution_id = GetStringColumn(record, 5);
  info->created_at = GetInt64Column(record, 6);

  auto items_callback =
      std::bind(&DatabaseSKUOrder::OnGetRecordItems, this, _1,
                std::make_shared<mojom::SKUOrderPtr>(info->Clone()), callback);
  items_.GetRecordsByOrderId(info->order_id, items_callback);
}

void DatabaseSKUOrder::OnGetRecordItems(
    std::vector<mojom::SKUOrderItemPtr> list,
    std::shared_ptr<mojom::SKUOrderPtr> shared_order,
    GetSKUOrderCallback callback) {
  if (!shared_order) {
    engine_->Log(FROM_HERE) << "Order is null";
    callback({});
    return;
  }

  (*shared_order)->items = std::move(list);
  callback(std::move(*shared_order));
}

void DatabaseSKUOrder::GetRecordByContributionId(
    const std::string& contribution_id,
    GetSKUOrderCallback callback) {
  if (contribution_id.empty()) {
    engine_->Log(FROM_HERE) << "Contribution id is empty";
    callback({});
    return;
  }
  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT order_id, total_amount, merchant_id, location, status, "
      "contribution_id, created_at FROM %s WHERE contribution_id = ?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, contribution_id);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseSKUOrder::OnGetRecord, base::Unretained(this),
                     std::move(callback)));
}

void DatabaseSKUOrder::SaveContributionIdForSKUOrder(
    const std::string& order_id,
    const std::string& contribution_id,
    LegacyResultCallback callback) {
  if (order_id.empty() || contribution_id.empty()) {
    engine_->Log(FROM_HERE) << "Order/contribution id is empty " << order_id
                            << "/" << contribution_id;
    callback(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET contribution_id = ? WHERE order_id = ?", kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, contribution_id);
  BindString(command.get(), 1, order_id);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

}  // namespace database
}  // namespace brave_rewards::internal
