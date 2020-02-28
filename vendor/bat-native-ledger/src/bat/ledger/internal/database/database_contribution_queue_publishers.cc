/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_contribution_queue_publishers.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char table_name_[] = "contribution_queue_publishers";
const char parent_table_name_[] = "contribution_queue";

}  // namespace

DatabaseContributionQueuePublishers::DatabaseContributionQueuePublishers(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseContributionQueuePublishers::
~DatabaseContributionQueuePublishers() = default;

bool DatabaseContributionQueuePublishers::CreateTableV9(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "contribution_queue_id INTEGER NOT NULL,"
        "publisher_key TEXT NOT NULL,"
        "amount_percent DOUBLE NOT NULL,"
        "CONSTRAINT fk_%s_publisher_key "
        "    FOREIGN KEY (publisher_key) "
        "    REFERENCES publisher_info (publisher_id),"
        "CONSTRAINT fk_%s_id "
        "    FOREIGN KEY (contribution_queue_id) "
        "    REFERENCES %s (contribution_queue_id) "
        "    ON DELETE CASCADE"
      ")",
      table_name_,
      table_name_,
      table_name_,
      parent_table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseContributionQueuePublishers::CreateTableV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "contribution_queue_id INTEGER NOT NULL,"
        "publisher_key TEXT NOT NULL,"
        "amount_percent DOUBLE NOT NULL"
      ")",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseContributionQueuePublishers::CreateIndexV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  bool success =
      this->InsertIndex(transaction, table_name_, "contribution_queue_id");

  if (!success) {
    return false;
  }

  return this->InsertIndex(transaction, table_name_, "publisher_key");
}

bool DatabaseContributionQueuePublishers::Migrate(
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

bool DatabaseContributionQueuePublishers::MigrateToV9(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, table_name_)) {
    return false;
  }

  if (!CreateTableV9(transaction)) {
    return false;
  }

  return true;
}

bool DatabaseContributionQueuePublishers::MigrateToV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(transaction, table_name_, temp_table_name)) {
    return false;
  }

  if (!CreateTableV15(transaction)) {
    return false;
  }

  if (!CreateIndexV15(transaction)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "contribution_queue_id", "contribution_queue_id" },
    { "publisher_key", "publisher_key" },
    { "amount_percent", "amount_percent" }
  };

  if (!MigrateDBTable(
      transaction,
      temp_table_name,
      table_name_,
      columns,
      true)) {
    return false;
  }
  return true;
}

void DatabaseContributionQueuePublishers::InsertOrUpdate(
    const uint64_t id,
    ledger::ContributionQueuePublisherList list,
    ledger::ResultCallback callback) {
  if (id == 0 || list.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(contribution_queue_id, publisher_key, amount_percent) VALUES (?, ?, ?)",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  for (const auto& publisher : list) {
    BindInt64(command.get(), 0, id);
    BindString(command.get(), 1, publisher->publisher_key);
    BindDouble(command.get(), 2, publisher->amount_percent);

    transaction->commands.push_back(command->Clone());
  }

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseContributionQueuePublishers::GetRecordsByQueueId(
    const uint64_t queue_id,
    ContributionQueuePublishersListCallback callback) {
  if (queue_id == 0) {
    callback({});
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT publisher_key, amount_percent "
      "FROM %s WHERE contribution_queue_id = ?",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindInt64(command.get(), 0, queue_id);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseContributionQueuePublishers::OnGetRecordsByQueueId,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseContributionQueuePublishers::OnGetRecordsByQueueId(
    ledger::DBCommandResponsePtr response,
    ContributionQueuePublishersListCallback callback) {
  if (!response
      || response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback({});
    return;
  }

  ledger::ContributionQueuePublisherList list;
  for (auto const& record : response->result->get_records()) {
    auto info = ledger::ContributionQueuePublisher::New();
    auto* record_pointer = record.get();

    info->publisher_key = GetStringColumn(record_pointer, 0);
    info->amount_percent = GetDoubleColumn(record_pointer, 1);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabaseContributionQueuePublishers::DeleteRecordsByQueueId(
    ledger::DBTransaction* transaction,
    const uint64_t queue_id) {
  DCHECK(transaction);

  if (queue_id == 0) {
    return;
  }

  const std::string query = base::StringPrintf(
      "DELETE FROM %s WHERE contribution_queue_id = ?",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt64(command.get(), 0, queue_id);

  transaction->commands.push_back(std::move(command));
}

}  // namespace braveledger_database
