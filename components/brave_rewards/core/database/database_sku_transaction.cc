/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/database/database_sku_transaction.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::database {

namespace {

constexpr char kTableName[] = "sku_transaction";

}  // namespace

DatabaseSKUTransaction::DatabaseSKUTransaction(RewardsEngine& engine)
    : DatabaseTable(engine) {}

DatabaseSKUTransaction::~DatabaseSKUTransaction() = default;

void DatabaseSKUTransaction::InsertOrUpdate(
    mojom::SKUTransactionPtr transaction,
    ResultCallback callback) {
  if (!transaction) {
    engine_->Log(FROM_HERE) << "Transcation is null";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto db_transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(transaction_id, order_id, external_transaction_id, type, amount, "
      "status) "
      "VALUES (?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRun;
  command->command = query;

  BindString(command.get(), 0, transaction->transaction_id);
  BindString(command.get(), 1, transaction->order_id);
  BindString(command.get(), 2, transaction->external_transaction_id);
  BindInt(command.get(), 3, static_cast<int>(transaction->type));
  BindDouble(command.get(), 4, transaction->amount);
  BindInt(command.get(), 5, static_cast<int>(transaction->status));

  db_transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(db_transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseSKUTransaction::SaveExternalTransaction(
    const std::string& transaction_id,
    const std::string& external_transaction_id,
    ResultCallback callback) {
  if (transaction_id.empty() || external_transaction_id.empty()) {
    engine_->Log(FROM_HERE)
        << "Data is empty " << transaction_id << "/" << external_transaction_id;
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET "
      "external_transaction_id = ?, status = ? WHERE transaction_id = ?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRun;
  command->command = query;

  BindString(command.get(), 0, external_transaction_id);
  BindInt(command.get(), 1,
          static_cast<int>(mojom::SKUTransactionStatus::COMPLETED));
  BindString(command.get(), 2, transaction_id);

  auto transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseSKUTransaction::GetRecordByOrderId(
    const std::string& order_id,
    GetSKUTransactionCallback callback) {
  if (order_id.empty()) {
    engine_->Log(FROM_HERE) << "Order id is empty";
    std::move(callback).Run(
        base::unexpected(GetSKUTransactionError::kDatabaseError));
    return;
  }
  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT transaction_id, order_id, external_transaction_id, amount, type, "
      "status FROM %s WHERE order_id = ?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRead;
  command->command = query;

  BindString(command.get(), 0, order_id);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kDouble,
                              mojom::DBCommand::RecordBindingType::kInt,
                              mojom::DBCommand::RecordBindingType::kInt};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseSKUTransaction::OnGetRecord,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseSKUTransaction::OnGetRecord(GetSKUTransactionCallback callback,
                                         mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::kSuccess) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    std::move(callback).Run(
        base::unexpected(GetSKUTransactionError::kDatabaseError));
    return;
  }

  if (response->records.empty()) {
    std::move(callback).Run(
        base::unexpected(GetSKUTransactionError::kTransactionNotFound));
    return;
  }

  if (response->records.size() > 1) {
    engine_->Log(FROM_HERE)
        << "Record size is not correct: " << response->records.size();
    std::move(callback).Run(
        base::unexpected(GetSKUTransactionError::kDatabaseError));
    return;
  }

  auto* record = response->records[0].get();

  auto info = mojom::SKUTransaction::New();
  info->transaction_id = GetStringColumn(record, 0);
  info->order_id = GetStringColumn(record, 1);
  info->external_transaction_id = GetStringColumn(record, 2);
  info->amount = GetDoubleColumn(record, 3);
  info->type = SKUTransactionTypeFromInt(GetIntColumn(record, 4));
  info->status = SKUTransactionStatusFromInt(GetIntColumn(record, 5));

  std::move(callback).Run(std::move(info));
}

}  // namespace brave_rewards::internal::database
