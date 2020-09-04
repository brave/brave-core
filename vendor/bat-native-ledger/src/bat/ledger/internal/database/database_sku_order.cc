/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_sku_order.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace ledger {
namespace database {

namespace {

const char kTableName[] = "sku_order";

}  // namespace

DatabaseSKUOrder::DatabaseSKUOrder(
    LedgerImpl* ledger) :
    DatabaseTable(ledger),
    items_(std::make_unique<DatabaseSKUOrderItems>(ledger)) {
}

DatabaseSKUOrder::~DatabaseSKUOrder() = default;

void DatabaseSKUOrder::InsertOrUpdate(
    type::SKUOrderPtr order,
    ledger::ResultCallback callback) {
  if (!order) {
    BLOG(1, "Order is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(order_id, total_amount, merchant_id, location, status, "
      "contribution_id) "
      "VALUES (?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
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

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseSKUOrder::UpdateStatus(
    const std::string& order_id,
    const type::SKUOrderStatus status,
    ledger::ResultCallback callback) {
  if (order_id.empty()) {
    BLOG(1, "Order id is empty");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ? WHERE order_id = ?",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(status));
  BindString(command.get(), 1, order_id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseSKUOrder::GetRecord(
    const std::string& order_id,
    GetSKUOrderCallback callback) {
  if (order_id.empty()) {
    BLOG(1, "Order id is empty");
    callback({});
    return;
  }

  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT order_id, total_amount, merchant_id, location, status, "
      "created_at FROM %s WHERE order_id = ?",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, order_id);

  command->record_bindings = {
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::DOUBLE_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::INT_TYPE,
      type::DBCommand::RecordBindingType::INT64_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabaseSKUOrder::OnGetRecord,
      this,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseSKUOrder::OnGetRecord(
    type::DBCommandResponsePtr response,
    GetSKUOrderCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  if (response->result->get_records().size() != 1) {
    BLOG(1, "Record size is not correct: " <<
        response->result->get_records().size());
    callback({});
    return;
  }

  auto* record = response->result->get_records()[0].get();
  auto info = type::SKUOrder::New();
  info->order_id = GetStringColumn(record, 0);
  info->total_amount = GetDoubleColumn(record, 1);
  info->merchant_id = GetStringColumn(record, 2);
  info->location = GetStringColumn(record, 3);
  info->status = static_cast<type::SKUOrderStatus>(GetIntColumn(record, 4));
  info->created_at = GetInt64Column(record, 5);

  auto items_callback = std::bind(&DatabaseSKUOrder::OnGetRecordItems,
      this,
      _1,
      std::make_shared<type::SKUOrderPtr>(info->Clone()),
      callback);
  items_->GetRecordsByOrderId(info->order_id, items_callback);
}

void DatabaseSKUOrder::OnGetRecordItems(
    type::SKUOrderItemList list,
    std::shared_ptr<type::SKUOrderPtr> shared_order,
    GetSKUOrderCallback callback) {
  if (!shared_order) {
    BLOG(1, "Order is null");
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
    BLOG(1, "Contribution id is empty");
    callback({});
    return;
  }
  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT order_id, total_amount, merchant_id, location, status, "
      "created_at FROM %s WHERE contribution_id = ?",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, contribution_id);

  command->record_bindings = {
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::DOUBLE_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::INT_TYPE,
      type::DBCommand::RecordBindingType::INT64_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabaseSKUOrder::OnGetRecord,
      this,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseSKUOrder::SaveContributionIdForSKUOrder(
    const std::string& order_id,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  if (order_id.empty() || contribution_id.empty()) {
    BLOG(1, "Order/contribution id is empty " <<
        order_id << "/" << contribution_id);
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET contribution_id = ? WHERE order_id = ?",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, contribution_id);
  BindString(command.get(), 1, order_id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

}  // namespace database
}  // namespace ledger
