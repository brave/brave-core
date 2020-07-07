/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_media_publisher_info.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char kTableName[] = "media_publisher_info";

}  // namespace

DatabaseMediaPublisherInfo::DatabaseMediaPublisherInfo(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseMediaPublisherInfo::~DatabaseMediaPublisherInfo() = default;

bool DatabaseMediaPublisherInfo::CreateTableV1(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "media_key TEXT NOT NULL PRIMARY KEY UNIQUE,"
        "publisher_id LONGVARCHAR NOT NULL,"
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      kTableName,
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseMediaPublisherInfo::CreateTableV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "media_key TEXT NOT NULL PRIMARY KEY UNIQUE,"
        "publisher_id LONGVARCHAR NOT NULL"
      ")",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseMediaPublisherInfo::CreateIndexV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  bool success = this->InsertIndex(transaction, kTableName, "media_key");

  if (!success) {
    return false;
  }

  return this->InsertIndex(transaction, kTableName, "publisher_id");
}

bool DatabaseMediaPublisherInfo::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 1: {
      return MigrateToV1(transaction);
    }
    case 15: {
      return MigrateToV15(transaction);
    }
    default: {
      return true;
    }
  }
}

bool DatabaseMediaPublisherInfo::MigrateToV1(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, kTableName)) {
    BLOG(0, "Table couldn't be dropped");
    return false;
  }

  if (!CreateTableV1(transaction)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  return true;
}

bool DatabaseMediaPublisherInfo::MigrateToV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      kTableName);

  if (!RenameDBTable(transaction, kTableName, temp_table_name)) {
    BLOG(0, "Table couldn't be renamed");
    return false;
  }

  if (!CreateTableV15(transaction)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  if (!CreateIndexV15(transaction)) {
    BLOG(0, "Index couldn't be created");
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "media_key", "media_key" },
    { "publisher_id", "publisher_id" }
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

void DatabaseMediaPublisherInfo::InsertOrUpdate(
    const std::string& media_key,
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  if (media_key.empty() || publisher_key.empty()) {
    BLOG(1, "Data is empty " << media_key << "/" << publisher_key);
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s (media_key, publisher_id) VALUES (?, ?)",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, media_key);
  BindString(command.get(), 1, publisher_key);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseMediaPublisherInfo::GetRecord(
    const std::string& media_key,
    ledger::PublisherInfoCallback callback) {
  if (media_key.empty()) {
    BLOG(1, "Media key is empty");
    return callback(ledger::Result::LEDGER_ERROR, {});
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
      "pi.provider, spi.status, spi.updated_at, pi.excluded "
      "FROM %s as mpi "
      "INNER JOIN publisher_info AS pi ON mpi.publisher_id = pi.publisher_id "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE mpi.media_key=?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, media_key);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseMediaPublisherInfo::OnGetRecord,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseMediaPublisherInfo::OnGetRecord(
    ledger::DBCommandResponsePtr response,
    ledger::PublisherInfoCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(1, "Response is wrong");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  if (response->result->get_records().size() != 1) {
    BLOG(1, "Record size is not correct: " <<
        response->result->get_records().size());
    callback(ledger::Result::NOT_FOUND, {});
    return;
  }

  auto* record = response->result->get_records()[0].get();
  auto info = ledger::PublisherInfo::New();

  info->id = GetStringColumn(record, 0);
  info->name = GetStringColumn(record, 1);
  info->url = GetStringColumn(record, 2);
  info->favicon_url = GetStringColumn(record, 3);
  info->provider = GetStringColumn(record, 4);
  info->status =
      static_cast<ledger::mojom::PublisherStatus>(GetIntColumn(record, 5));
  info->status_updated_at = GetInt64Column(record, 6);
  info->excluded =
      static_cast<ledger::PublisherExclude>(GetIntColumn(record, 7));

  callback(ledger::Result::LEDGER_OK, std::move(info));
}

}  // namespace braveledger_database
