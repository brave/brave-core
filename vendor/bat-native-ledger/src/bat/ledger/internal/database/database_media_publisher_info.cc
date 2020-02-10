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

const char table_name_[] = "media_publisher_info";

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
      table_name_,
      table_name_);

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
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseMediaPublisherInfo::CreateIndexV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  bool success = this->InsertIndex(transaction, table_name_, "media_key");

  if (!success) {
    return false;
  }

  return this->InsertIndex(transaction, table_name_, "publisher_id");
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

  if (!DropTable(transaction, table_name_)) {
    return false;
  }

  if (!CreateTableV1(transaction)) {
    return false;
  }

  return true;
}

bool DatabaseMediaPublisherInfo::MigrateToV15(
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
    { "media_key", "media_key" },
    { "publisher_id", "publisher_id" }
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

void DatabaseMediaPublisherInfo::InsertOrUpdate(
    const std::string& media_key,
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  if (media_key.empty() || publisher_key.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s (media_key, publisher_id) VALUES (?, ?)",
      table_name_);

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
    return callback(ledger::Result::LEDGER_ERROR, {});
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
      "pi.provider, spi.status, pi.excluded "
      "FROM %s as mpi "
      "INNER JOIN publisher_info AS pi ON mpi.publisher_id = pi.publisher_id "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE mpi.media_key=?",
      table_name_);

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
  if (!response
      || response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  if (response->result->get_records().size() != 1) {
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
  info->excluded =
      static_cast<ledger::PublisherExclude>(GetIntColumn(record, 6));

  callback(ledger::Result::LEDGER_OK, std::move(info));
}

}  // namespace braveledger_database
