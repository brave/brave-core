/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_server_publisher_links.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "server_publisher_links";

}  // namespace

namespace braveledger_database {

DatabaseServerPublisherLinks::DatabaseServerPublisherLinks(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseServerPublisherLinks::~DatabaseServerPublisherLinks() = default;

bool DatabaseServerPublisherLinks::CreateTableV7(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "publisher_key LONGVARCHAR NOT NULL,"
      "provider TEXT,"
      "link TEXT,"
      "CONSTRAINT %s_unique "
      "    UNIQUE (publisher_key, provider) "
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

bool DatabaseServerPublisherLinks::CreateTableV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "publisher_key LONGVARCHAR NOT NULL,"
      "provider TEXT,"
      "link TEXT,"
      "CONSTRAINT %s_unique "
      "    UNIQUE (publisher_key, provider)"
      ")",
      kTableName,
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseServerPublisherLinks::CreateIndexV7(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "publisher_key");
}

bool DatabaseServerPublisherLinks::CreateIndexV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "publisher_key");
}

bool DatabaseServerPublisherLinks::Migrate(
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
    default: {
      return true;
    }
  }
}


bool DatabaseServerPublisherLinks::MigrateToV7(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, kTableName)) {
    return false;
  }

  if (!CreateTableV7(transaction)) {
    return false;
  }

  if (!CreateIndexV7(transaction)) {
    return false;
  }

  return true;
}

bool DatabaseServerPublisherLinks::MigrateToV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      kTableName);

  if (!RenameDBTable(transaction, kTableName, temp_table_name)) {
    return false;
  }

  const std::string query =
      "DROP INDEX IF EXISTS server_publisher_links_publisher_key_index;";
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
    { "publisher_key", "publisher_key" },
    { "provider", "provider" },
    { "link", "link" }
  };

  if (!MigrateDBTable(
      transaction,
      temp_table_name,
      kTableName,
      columns,
      true)) {
    return false;
  }

  return true;
}

void DatabaseServerPublisherLinks::InsertOrUpdate(
    ledger::DBTransaction* transaction,
    const ledger::PublisherBanner& info) {
  DCHECK(transaction);

  // It's ok if links are empty
  if (info.links.empty()) {
    return;
  }

  for (const auto& link : info.links) {
    if (link.second.empty()) {
      continue;
    }

    const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_key, provider, link) "
      "VALUES (?, ?, ?)",
      kTableName);

    auto command = ledger::DBCommand::New();
    command->type = ledger::DBCommand::Type::RUN;
    command->command = query;

    BindString(command.get(), 0, info.publisher_key);
    BindString(command.get(), 1, link.first);
    BindString(command.get(), 2, link.second);
    transaction->commands.push_back(std::move(command));
  }
}

void DatabaseServerPublisherLinks::GetRecord(
    const std::string& publisher_key,
    ServerPublisherLinksCallback callback) {
  auto transaction = ledger::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "SELECT provider, link FROM %s WHERE publisher_key=?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseServerPublisherLinks::OnGetRecord,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseServerPublisherLinks::OnGetRecord(
    ledger::DBCommandResponsePtr response,
    ServerPublisherLinksCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback({});
    return;
  }

  std::map<std::string, std::string> links;
  for (auto const& record : response->result->get_records()) {
    auto* record_pointer = record.get();
    const auto pair = std::make_pair(
        GetStringColumn(record_pointer, 0),
        GetStringColumn(record_pointer, 1));
    links.insert(pair);
  }

  callback(links);
}

}  // namespace braveledger_database
