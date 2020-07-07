/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_server_publisher_amounts.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "server_publisher_amounts";

}  // namespace

namespace braveledger_database {

DatabaseServerPublisherAmounts::DatabaseServerPublisherAmounts(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseServerPublisherAmounts::~DatabaseServerPublisherAmounts() = default;

bool DatabaseServerPublisherAmounts::CreateTableV7(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "publisher_key LONGVARCHAR NOT NULL,"
      "amount DOUBLE DEFAULT 0 NOT NULL,"
      "CONSTRAINT %s_unique "
      "    UNIQUE (publisher_key, amount) "
      "CONSTRAINT fk_%s_publisher_key"
      "    FOREIGN KEY (publisher_key)"
      "    REFERENCES server_publisher_info (publisher_key)"
      "    ON DELETE CASCADE"
      ")",
      kTableName,
      kTableName,
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseServerPublisherAmounts::CreateTableV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "publisher_key LONGVARCHAR NOT NULL,"
      "amount DOUBLE DEFAULT 0 NOT NULL,"
      "CONSTRAINT %s_unique "
      "    UNIQUE (publisher_key, amount)"
      ")",
      kTableName,
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseServerPublisherAmounts::CreateIndexV7(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "publisher_key");
}

bool DatabaseServerPublisherAmounts::CreateIndexV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "publisher_key");
}

bool DatabaseServerPublisherAmounts::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 7: {
      return MigrateToV7(transaction);
    }
    case 15: {
      return MigrateToV15(transaction);
    }
    case 28: {
      return MigrateToV28(transaction);
    }
    default: {
      return true;
    }
  }
}

bool DatabaseServerPublisherAmounts::MigrateToV7(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, kTableName)) {
    BLOG(0, "Table couldn't be dropped");
    return false;
  }

  if (!CreateTableV7(transaction)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  if (!CreateIndexV7(transaction)) {
    BLOG(0, "Index couldn't be created");
    return false;
  }

  return true;
}

bool DatabaseServerPublisherAmounts::MigrateToV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      kTableName);

  if (!RenameDBTable(transaction, kTableName, temp_table_name)) {
    BLOG(0, "Table couldn't be renamed");
    return false;
  }

  const std::string query =
      "DROP INDEX IF EXISTS server_publisher_amounts_publisher_key_index;";
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!CreateTableV15(transaction)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  if (!CreateIndexV15(transaction)) {
    BLOG(0, "Index couldn't be created");
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "publisher_key", "publisher_key" },
    { "amount", "amount" }
  };

  if (!MigrateDBTable(
      transaction,
      temp_table_name,
      kTableName,
      columns,
      true)) {
    BLOG(0, "Table migration failed");
    return false;
  }

  return true;
}

bool DatabaseServerPublisherAmounts::MigrateToV28(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = base::StringPrintf("DELETE FROM %s", kTableName);
  transaction->commands.push_back(std::move(command));
  return true;
}

void DatabaseServerPublisherAmounts::InsertOrUpdate(
    ledger::DBTransaction* transaction,
    const ledger::ServerPublisherInfo& server_info) {
  DCHECK(transaction && !server_info.publisher_key.empty());
  if (!server_info.banner || server_info.banner->amounts.empty()) {
    return;
  }

  std::string value_list;
  for (const auto& amount : server_info.banner->amounts) {
    value_list += base::StringPrintf(
        R"(("%s",%g),)",
        server_info.publisher_key.c_str(),
        amount);
  }

  DCHECK(!value_list.empty());

  // Remove trailing comma
  value_list.pop_back();

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "INSERT OR REPLACE INTO %s VALUES %s",
      kTableName,
      value_list.c_str());

  transaction->commands.push_back(std::move(command));
}

void DatabaseServerPublisherAmounts::DeleteRecords(
    ledger::DBTransaction* transaction,
    const std::string& publisher_key_list) {
  DCHECK(transaction);
  if (publisher_key_list.empty()) {
    return;
  }

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "DELETE FROM %s WHERE publisher_key IN (%s)",
      kTableName,
      publisher_key_list.c_str());

  transaction->commands.push_back(std::move(command));
}

void DatabaseServerPublisherAmounts::GetRecord(
    const std::string& publisher_key,
    ServerPublisherAmountsCallback callback) {
  if (publisher_key.empty()) {
    BLOG(1, "Publisher key is empty");
    callback({});
    return;
  }
  auto transaction = ledger::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "SELECT amount FROM %s WHERE publisher_key=?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseServerPublisherAmounts::OnGetRecord,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseServerPublisherAmounts::OnGetRecord(
    ledger::DBCommandResponsePtr response,
    ServerPublisherAmountsCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  std::vector<double> amounts;
  for (auto const& record : response->result->get_records()) {
    amounts.push_back(GetDoubleColumn(record.get(), 0));
  }

  callback(amounts);
}

}  // namespace braveledger_database
