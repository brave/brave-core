/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/bind_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/database/database_contribution_queue.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char kTableName[] = "contribution_queue";

}  // namespace

DatabaseContributionQueue::DatabaseContributionQueue(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger),
    publishers_(std::make_unique<DatabaseContributionQueuePublishers>(ledger)) {
}

DatabaseContributionQueue::~DatabaseContributionQueue() = default;

bool DatabaseContributionQueue::CreateTableV9(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "contribution_queue_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
        "type INTEGER NOT NULL,"
        "amount DOUBLE NOT NULL,"
        "partial INTEGER NOT NULL DEFAULT 0,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL"
      ")",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseContributionQueue::CreateTableV23(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "contribution_queue_id TEXT PRIMARY KEY NOT NULL,"
        "type INTEGER NOT NULL,"
        "amount DOUBLE NOT NULL,"
        "partial INTEGER NOT NULL DEFAULT 0,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL"
      ")",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseContributionQueue::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 9: {
      return MigrateToV9(transaction);
    }
    case 15: {
      return MigrateToV15(transaction);
    }
    case 23: {
      return MigrateToV23(transaction);
    }
    case 24: {
      return MigrateToV24(transaction);
    }
    default: {
      return true;
    }
  }
}

bool DatabaseContributionQueue::MigrateToV9(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, kTableName)) {
    BLOG(0, "Table couldn't be dropped");
    return false;
  }

  if (!CreateTableV9(transaction)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  if (!publishers_->Migrate(transaction, 9)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  return true;
}

bool DatabaseContributionQueue::MigrateToV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return publishers_->Migrate(transaction, 15);
}

bool DatabaseContributionQueue::MigrateToV23(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      kTableName);

  if (!RenameDBTable(transaction, kTableName, temp_table_name)) {
    BLOG(0, "Table couldn't be renamed");
    return false;
  }

  if (!CreateTableV23(transaction)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  // Migrate the contribution_queue table
  const std::string query = base::StringPrintf(
      "INSERT INTO %s "
      "(contribution_queue_id, type, amount, partial, created_at) "
      "SELECT CAST(contribution_queue_id AS TEXT), type, amount, partial, "
      "created_at FROM %s",
      kTableName,
      temp_table_name.c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!DropTable(transaction, temp_table_name)) {
    BLOG(0, "Table couldn't be dropped");
    return false;
  }

  if (!publishers_->Migrate(transaction, 23)) {
    BLOG(0, "Table couldn't be migrated");
    return false;
  }

  return true;
}

bool DatabaseContributionQueue::MigrateToV24(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "ALTER TABLE %s ADD completed_at TIMESTAMP NOT NULL DEFAULT 0",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!publishers_->Migrate(transaction, 24)) {
    BLOG(0, "Table couldn't be migrated");
    return false;
  }

  return true;
}

void DatabaseContributionQueue::InsertOrUpdate(
    ledger::ContributionQueuePtr info,
    ledger::ResultCallback callback) {
  if (!info) {
    BLOG(0, "Queue is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (info->id.empty()) {
    BLOG(0, "Queue id is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "INSERT OR REPLACE INTO %s (contribution_queue_id, type, amount, partial) "
    "VALUES (?, ?, ?, ?)",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->id);
  BindInt(command.get(), 1, static_cast<int>(info->type));
  BindDouble(command.get(), 2, info->amount);
  BindBool(command.get(), 3, info->partial);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseContributionQueue::OnInsertOrUpdate,
          this,
          _1,
          braveledger_bind_util::FromContributionQueueToString(info->Clone()),
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseContributionQueue::OnInsertOrUpdate(
    ledger::DBCommandResponsePtr response,
    const std::string& queue_string,
    ledger::ResultCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is not ok");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto queue =
      braveledger_bind_util::FromStringToContributionQueue(queue_string);

  if (!queue) {
    BLOG(0, "Queue is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  publishers_->InsertOrUpdate(
      queue->id,
      std::move(queue->publishers),
      callback);
  return;
}

void DatabaseContributionQueue::GetFirstRecord(
    ledger::GetFirstContributionQueueCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT contribution_queue_id, type, amount, partial "
      "FROM %s WHERE completed_at = 0 "
      "ORDER BY created_at ASC LIMIT 1",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseContributionQueue::OnGetFirstRecord,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseContributionQueue::OnGetFirstRecord(
    ledger::DBCommandResponsePtr response,
    ledger::GetFirstContributionQueueCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(nullptr);
    return;
  }

  if (response->result->get_records().size() != 1) {
    callback(nullptr);
    return;
  }

  auto* record = response->result->get_records()[0].get();

  auto info = ledger::ContributionQueue::New();
  info->id = GetStringColumn(record, 0);
  info->type = static_cast<ledger::RewardsType>(GetIntColumn(record, 1));
  info->amount = GetDoubleColumn(record, 2);
  info->partial = static_cast<bool>(GetIntColumn(record, 3));

  auto publishers_callback =
      std::bind(&DatabaseContributionQueue::OnGetPublishers,
          this,
          _1,
          braveledger_bind_util::FromContributionQueueToString(info->Clone()),
          callback);

  publishers_->GetRecordsByQueueId(info->id, publishers_callback);
}

void DatabaseContributionQueue::OnGetPublishers(
    ledger::ContributionQueuePublisherList list,
    const std::string& queue_string,
    ledger::GetFirstContributionQueueCallback callback) {
  auto queue =
      braveledger_bind_util::FromStringToContributionQueue(queue_string);

  if (!queue) {
    BLOG(0, "Queue is null");
    callback(nullptr);
    return;
  }

  queue->publishers = std::move(list);

  callback(std::move(queue));
}

void DatabaseContributionQueue::MarkRecordAsComplete(
    const std::string& id,
    ledger::ResultCallback callback) {
  if (id.empty()) {
    BLOG(1, "Id is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET completed_at = ? WHERE contribution_queue_id = ?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt64(command.get(), 0, braveledger_time_util::GetCurrentTimeStamp());
  BindString(command.get(), 1, id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

}  // namespace braveledger_database
