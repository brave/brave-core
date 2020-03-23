/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/bind_util.h"
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
    default: {
      return true;
    }
  }
}

bool DatabaseContributionQueue::MigrateToV9(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, kTableName)) {
    return false;
  }

  if (!CreateTableV9(transaction)) {
    return false;
  }

  if (!publishers_->Migrate(transaction, 9)) {
    return false;
  }

  return true;
}

bool DatabaseContributionQueue::MigrateToV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return publishers_->Migrate(transaction, 15);
}

void DatabaseContributionQueue::InsertOrUpdate(
    ledger::ContributionQueuePtr info,
    ledger::ResultCallback callback) {
  if (!info) {
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

  bool new_entry = false;
  if (info->id != 0) {
    BindInt64(command.get(), 0, info->id);
  } else {
    BindNull(command.get(), 0);
    new_entry = true;
  }

  BindInt(command.get(), 1, static_cast<int>(info->type));
  BindDouble(command.get(), 2, info->amount);
  BindBool(command.get(), 3, info->partial);

  transaction->commands.push_back(std::move(command));

  if (new_entry) {
    const std::string new_query = base::StringPrintf(
        "SELECT contribution_queue_id FROM %s "
        "ORDER BY contribution_queue_id DESC LIMIT 1",
    kTableName);

    auto new_command = ledger::DBCommand::New();
    new_command->type = ledger::DBCommand::Type::READ;
    new_command->command = new_query;

    new_command->record_bindings = {
      ledger::DBCommand::RecordBindingType::INT64_TYPE
    };

    transaction->commands.push_back(std::move(new_command));
  }

  auto transaction_callback =
      std::bind(&DatabaseContributionQueue::OnInsertOrUpdate,
          this,
          _1,
          braveledger_bind_util::FromContributionQueueToString(info->Clone()),
          info->id,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseContributionQueue::OnInsertOrUpdate(
    ledger::DBCommandResponsePtr response,
    const std::string& queue_string,
    const uint64_t id,
    ledger::ResultCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto queue =
      braveledger_bind_util::FromStringToContributionQueue(queue_string);

  if (!queue) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (id != 0) {
    publishers_->InsertOrUpdate(id, std::move(queue->publishers), callback);
    return;
  }

  if (response->result->get_records().size() != 1) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto* record = response->result->get_records()[0].get();

  publishers_->InsertOrUpdate(
      GetInt64Column(record, 0),
      std::move(queue->publishers),
      callback);
}

void DatabaseContributionQueue::GetFirstRecord(
    ledger::GetFirstContributionQueueCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT contribution_queue_id, type, amount, partial "
      "FROM %s ORDER BY contribution_queue_id ASC LIMIT 1",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
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
    callback(nullptr);
    return;
  }

  if (response->result->get_records().size() != 1) {
    callback(nullptr);
    return;
  }

  auto* record = response->result->get_records()[0].get();

  auto info = ledger::ContributionQueue::New();
  info->id = GetInt64Column(record, 0);
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
    callback(nullptr);
    return;
  }

  queue->publishers = std::move(list);

  callback(std::move(queue));
}

void DatabaseContributionQueue::DeleteRecord(
    const uint64_t id,
    ledger::ResultCallback callback) {
  if (id == 0) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "DELETE FROM %s WHERE contribution_queue_id = ?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt64(command.get(), 0, id);

  transaction->commands.push_back(std::move(command));

  publishers_->DeleteRecordsByQueueId(transaction.get(), id);

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

}  // namespace braveledger_database
