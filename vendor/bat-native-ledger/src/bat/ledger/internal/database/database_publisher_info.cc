/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/database/database_publisher_info.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "publisher_info";

}  // namespace

namespace braveledger_database {

DatabasePublisherInfo::DatabasePublisherInfo(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabasePublisherInfo::~DatabasePublisherInfo() = default;

bool DatabasePublisherInfo::CreateTableV1(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,"
        "verified BOOLEAN DEFAULT 0 NOT NULL,"
        "excluded INTEGER DEFAULT 0 NOT NULL,"
        "name TEXT NOT NULL,"
        "favIcon TEXT NOT NULL,"
        "url TEXT NOT NULL,"
        "provider TEXT NOT NULL"
      ")",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabasePublisherInfo::CreateTableV7(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,"
        "excluded INTEGER DEFAULT 0 NOT NULL,"
        "name TEXT NOT NULL,"
        "favIcon TEXT NOT NULL,"
        "url TEXT NOT NULL,"
        "provider TEXT NOT NULL"
      ")",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabasePublisherInfo::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 1: {
      return MigrateToV1(transaction);
    }
    case 7: {
      return MigrateToV7(transaction);
    }
    default: {
      return true;
    }
  }
}

bool DatabasePublisherInfo::MigrateToV1(ledger::DBTransaction* transaction) {
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

bool DatabasePublisherInfo::MigrateToV7(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_old",
      kTableName);

  if (!RenameDBTable(transaction, kTableName, temp_table_name)) {
    BLOG(0, "Table couldn't be renamed");
    return false;
  }

  if (!CreateTableV7(transaction)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "publisher_id", "publisher_id" },
    { "excluded", "excluded" },
    { "name", "name" },
    { "favIcon", "favIcon" },
    { "url", "url" },
    { "provider", "provider" }
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

void DatabasePublisherInfo::InsertOrUpdate(
    ledger::PublisherInfoPtr info,
    ledger::ResultCallback callback) {
  if (!info || info->id.empty()) {
    BLOG(1, "Info is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_id, excluded, name, url, provider, favIcon) "
      "VALUES (?, ?, ?, ?, ?, "
      "(SELECT IFNULL( "
      "(SELECT favicon FROM %s "
      "WHERE publisher_id = ?), \"\")));",
      kTableName,
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->id);
  BindInt(command.get(), 1, static_cast<int>(info->excluded));
  BindString(command.get(), 2, info->name);
  BindString(command.get(), 3, info->url);
  BindString(command.get(), 4, info->provider);
  BindString(command.get(), 5, info->id);

  transaction->commands.push_back(std::move(command));

  std::string favicon = info->favicon_url;
  if (!favicon.empty() && !info->provider.empty()) {
    const std::string query_icon = base::StringPrintf(
        "UPDATE %s SET favIcon = ? WHERE publisher_id = ?;",
        kTableName);

    auto command_icon = ledger::DBCommand::New();
    command_icon->type = ledger::DBCommand::Type::RUN;
    command_icon->command = query_icon;

    if (favicon == ledger::kClearFavicon) {
      favicon.clear();
    }

    BindString(command_icon.get(), 0, favicon);
    BindString(command_icon.get(), 1, info->id);

    transaction->commands.push_back(std::move(command_icon));
  }

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePublisherInfo::GetRecord(
    const std::string& publisher_key,
    ledger::PublisherInfoCallback callback) {
  if (publisher_key.empty()) {
    BLOG(1, "Publisher key is empty");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, pi.provider, "
    "spi.status, spi.updated_at, pi.excluded "
    "FROM %s as pi "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = pi.publisher_id "
    "WHERE publisher_id=?",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabasePublisherInfo::OnGetRecord,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePublisherInfo::OnGetRecord(
    ledger::DBCommandResponsePtr response,
    ledger::PublisherInfoCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
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
  info->status = static_cast<ledger::mojom::PublisherStatus>(
      GetInt64Column(record, 5));
  info->status_updated_at = GetInt64Column(record, 6);
  info->excluded = static_cast<ledger::PublisherExclude>(
      GetIntColumn(record, 7));

  callback(ledger::Result::LEDGER_OK, std::move(info));
}

void DatabasePublisherInfo::GetPanelRecord(
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoCallback callback) {
  if (!filter || filter->id.empty()) {
    BLOG(1, "Filter is empty");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
    "pi.provider, spi.status, pi.excluded, "
    "("
    "  SELECT IFNULL(percent, 0) FROM activity_info WHERE "
    "  publisher_id = ? AND reconcile_stamp = ? "
    ") as percent "
    "FROM %s AS pi "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = pi.publisher_id "
    "WHERE pi.publisher_id = ? LIMIT 1",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, filter->id);
  BindInt64(command.get(), 1, filter->reconcile_stamp);
  BindString(command.get(), 2, filter->id);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(
      &DatabasePublisherInfo::OnGetPanelRecord,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePublisherInfo::OnGetPanelRecord(
    ledger::DBCommandResponsePtr response,
    ledger::PublisherInfoCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
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
      static_cast<ledger::mojom::PublisherStatus>(GetInt64Column(record, 5));
  info->excluded = static_cast<ledger::PublisherExclude>(
      GetIntColumn(record, 6));
  info->percent = GetIntColumn(record, 7);

  callback(ledger::Result::LEDGER_OK, std::move(info));
}

void DatabasePublisherInfo::RestorePublishers(ledger::ResultCallback callback) {
  auto transaction = ledger::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "UPDATE %s SET excluded=? WHERE excluded=?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt(
      command.get(),
      0,
      static_cast<int>(ledger::PublisherExclude::DEFAULT));
  BindInt(
      command.get(),
      1,
      static_cast<int>(ledger::PublisherExclude::EXCLUDED));

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePublisherInfo::GetExcludedList(
    ledger::PublisherInfoListCallback callback) {
  auto transaction = ledger::DBTransaction::New();
  const std::string query = base::StringPrintf(
    "SELECT pi.publisher_id, spi.status, pi.name,"
    "pi.favicon, pi.url, pi.provider "
    "FROM %s as pi "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = pi.publisher_id "
    "WHERE pi.excluded = 1",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(
      &DatabasePublisherInfo::OnGetExcludedList,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePublisherInfo::OnGetExcludedList(
    ledger::DBCommandResponsePtr response,
    ledger::PublisherInfoListCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  ledger::PublisherInfoList list;
  for (auto const& record : response->result->get_records()) {
    auto info = ledger::PublisherInfo::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->status = static_cast<ledger::mojom::PublisherStatus>(
        GetInt64Column(record_pointer, 1));
    info->name = GetStringColumn(record_pointer, 2);
    info->favicon_url = GetStringColumn(record_pointer, 3);
    info->url = GetStringColumn(record_pointer, 4);
    info->provider = GetStringColumn(record_pointer, 5);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

}  // namespace braveledger_database
