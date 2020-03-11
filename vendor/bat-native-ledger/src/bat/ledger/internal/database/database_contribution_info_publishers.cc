/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_contribution_info_publishers.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char table_name_[] = "contribution_info_publishers";

}  // namespace

DatabaseContributionInfoPublishers::DatabaseContributionInfoPublishers(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseContributionInfoPublishers::
~DatabaseContributionInfoPublishers() = default;

bool DatabaseContributionInfoPublishers::CreateTableV11(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "contribution_id TEXT NOT NULL,"
        "publisher_key TEXT NOT NULL,"
        "total_amount DOUBLE NOT NULL,"
        "contributed_amount DOUBLE,"
        "CONSTRAINT fk_contribution_info_publishers_contribution_id "
        "    FOREIGN KEY (contribution_id) "
        "    REFERENCES contribution_info (contribution_id) "
        "    ON DELETE CASCADE,"
        "CONSTRAINT fk_contribution_info_publishers_publisher_id "
        "    FOREIGN KEY (publisher_key) "
        "    REFERENCES publisher_info (publisher_id)"
      ")",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseContributionInfoPublishers::CreateTableV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "contribution_id TEXT NOT NULL,"
        "publisher_key TEXT NOT NULL,"
        "total_amount DOUBLE NOT NULL,"
        "contributed_amount DOUBLE"
      ")",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseContributionInfoPublishers::CreateIndexV11(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  bool success = this->InsertIndex(transaction, table_name_, "contribution_id");

  if (!success) {
    return false;
  }

  return this->InsertIndex(transaction, table_name_, "publisher_key");
}

bool DatabaseContributionInfoPublishers::CreateIndexV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  bool success = this->InsertIndex(transaction, table_name_, "contribution_id");

  if (!success) {
    return false;
  }

  return this->InsertIndex(transaction, table_name_, "publisher_key");
}

bool DatabaseContributionInfoPublishers::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 11: {
      return MigrateToV11(transaction);
    }
    case 15: {
      return MigrateToV15(transaction);
    }
    default: {
      return true;
    }
  }
}

bool DatabaseContributionInfoPublishers::MigrateToV11(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, table_name_)) {
    return false;
  }

  if (!CreateTableV11(transaction)) {
    return false;
  }

  if (!CreateIndexV11(transaction)) {
    return false;
  }

  return true;
}

bool DatabaseContributionInfoPublishers::MigrateToV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(transaction, table_name_, temp_table_name)) {
    return false;
  }

  const std::string query =
      "DROP INDEX IF EXISTS contribution_info_publishers_contribution_id_index;"
      " DROP INDEX IF EXISTS contribution_info_publishers_publisher_key_index;";
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!CreateTableV15(transaction)) {
    return false;
  }

  if (!CreateIndexV15(transaction)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "contribution_id", "contribution_id" },
    { "publisher_key", "publisher_key" },
    { "total_amount", "total_amount" },
    { "contributed_amount", "contributed_amount" }
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

void DatabaseContributionInfoPublishers::InsertOrUpdate(
    ledger::DBTransaction* transaction,
    ledger::ContributionInfoPtr info) {
  DCHECK(transaction);

  if (!info) {
    return;
  }

  const std::string query_delete = base::StringPrintf(
    "DELETE FROM %s WHERE contribution_id = ? AND publisher_key = ?",
    table_name_);

  const std::string query = base::StringPrintf(
    "INSERT INTO %s "
    "(contribution_id, publisher_key, total_amount, contributed_amount) "
    "VALUES (?, ?, ?, ?)",
    table_name_);

  for (const auto& publisher : info->publishers) {
    auto command_delete = ledger::DBCommand::New();
    command_delete->type = ledger::DBCommand::Type::RUN;
    command_delete->command = query_delete;

    BindString(command_delete.get(), 0, publisher->contribution_id);
    BindString(command_delete.get(), 1, publisher->publisher_key);
    transaction->commands.push_back(std::move(command_delete));

    auto command = ledger::DBCommand::New();
    command->type = ledger::DBCommand::Type::RUN;
    command->command = query;

    BindString(command.get(), 0, publisher->contribution_id);
    BindString(command.get(), 1, publisher->publisher_key);
    BindDouble(command.get(), 2, publisher->total_amount);
    BindDouble(command.get(), 3, publisher->contributed_amount);
    transaction->commands.push_back(std::move(command));
  }
}

void DatabaseContributionInfoPublishers::GetRecordByContributionList(
    const std::vector<std::string>& contribution_ids,
    ContributionPublisherListCallback callback) {
  if (contribution_ids.empty()) {
    callback({});
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT contribution_id, publisher_key, total_amount, contributed_amount "
    "FROM %s WHERE contribution_id IN (%s)",
    table_name_,
    GenerateStringInCase(contribution_ids).c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(
      &DatabaseContributionInfoPublishers::OnGetRecordByContributionList,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseContributionInfoPublishers::OnGetRecordByContributionList(
    ledger::DBCommandResponsePtr response,
    ContributionPublisherListCallback callback) {
  if (!response
      || response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback({});
    return;
  }

  ledger::ContributionPublisherList list;
  for (auto const& record : response->result->get_records()) {
    auto info = ledger::ContributionPublisher::New();
    auto* record_pointer = record.get();

    info->contribution_id = GetStringColumn(record_pointer, 0);
    info->publisher_key = GetStringColumn(record_pointer, 1);
    info->total_amount = GetDoubleColumn(record_pointer, 2);
    info->contributed_amount = GetDoubleColumn(record_pointer, 3);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabaseContributionInfoPublishers::GetContributionPublisherInfoMap(
    const std::vector<std::string>& contribution_ids,
    ContributionPublisherInfoMapCallback callback) {
  if (contribution_ids.empty()) {
    callback({});
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT cip.contribution_id, cip.publisher_key, cip.total_amount, "
    "pi.name, pi.url, pi.favIcon, spi.status, pi.provider "
    "FROM %s as cip "
    "INNER JOIN publisher_info AS pi ON cip.publisher_key = pi.publisher_id "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = cip.publisher_key "
    "WHERE cip.contribution_id IN (%s)",
    table_name_,
    GenerateStringInCase(contribution_ids).c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(
      &DatabaseContributionInfoPublishers::OnGetContributionPublisherInfoMap,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseContributionInfoPublishers::OnGetContributionPublisherInfoMap(
    ledger::DBCommandResponsePtr response,
    ContributionPublisherInfoMapCallback callback) {
  if (!response
      || response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback({});
    return;
  }

  ContributionPublisherInfoMap map;
  for (auto const& record : response->result->get_records()) {
    auto publisher = ledger::PublisherInfo::New();
    auto* record_pointer = record.get();

    publisher->id = GetStringColumn(record_pointer, 1);
    publisher->weight = GetDoubleColumn(record_pointer, 2);
    publisher->name = GetStringColumn(record_pointer, 3);
    publisher->url = GetStringColumn(record_pointer, 4);
    publisher->favicon_url = GetStringColumn(record_pointer, 5);
    publisher->status = static_cast<ledger::mojom::PublisherStatus>(
        GetInt64Column(record_pointer, 6));
    publisher->provider = GetStringColumn(record_pointer, 7);

    map.insert(std::make_pair(
        GetStringColumn(record_pointer, 0),
        std::move(publisher)));
  }

  callback(std::move(map));
}

void DatabaseContributionInfoPublishers::UpdateContributedAmount(
    const std::string& contribution_id,
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  if (contribution_id.empty() || publisher_key.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET contributed_amount="
      "(SELECT total_amount WHERE contribution_id = ? AND publisher_key = ?) "
      "WHERE contribution_id = ? AND publisher_key = ?;",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, contribution_id);
  BindString(command.get(), 1, publisher_key);
  BindString(command.get(), 2, contribution_id);
  BindString(command.get(), 3, publisher_key);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

}  // namespace braveledger_database
