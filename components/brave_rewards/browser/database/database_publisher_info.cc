/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "brave/components/brave_rewards/browser/database/database_publisher_info.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards {

namespace {
  const char* table_name_ = "publisher_info";
  const int minimum_version_ = 1;
}  // namespace

DatabasePublisherInfo::DatabasePublisherInfo(
    int current_db_version) :
    DatabaseTable(current_db_version) {
}

DatabasePublisherInfo::~DatabasePublisherInfo() = default;

bool DatabasePublisherInfo::Init(sql::Database* db) {
  if (GetCurrentDBVersion() < minimum_version_) {
    return true;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  bool success = CreateTable(db);
  if (!success) {
    return false;
  }

  success = CreateIndex(db);
  if (!success) {
    return false;
  }

  return transaction.Commit();
}

bool DatabasePublisherInfo::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  return CreateTableV7(db);
}

bool DatabasePublisherInfo::CreateTableV1(sql::Database* db) {
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
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabasePublisherInfo::CreateTableV7(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,"
        "excluded INTEGER DEFAULT 0 NOT NULL,"
        "name TEXT NOT NULL,"
        "favIcon TEXT NOT NULL,"
        "url TEXT NOT NULL,"
        "provider TEXT NOT NULL"
      ")",
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabasePublisherInfo::CreateIndex(sql::Database* db) {
  return true;
}

bool DatabasePublisherInfo::Migrate(sql::Database* db, const int target) {
  switch (target) {
    case 1: {
      return MigrateToV1(db);
    }
    case 7: {
      return MigrateToV7(db);
    }
    default: {
      NOTREACHED();
      return false;
    }
  }
}

bool DatabasePublisherInfo::MigrateToV1(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    DropTable(db, table_name_);
  }

  if (!CreateTableV1(db)) {
    return false;
  }

  return true;
}

bool DatabasePublisherInfo::MigrateToV7(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_old",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  if (!CreateTableV7(db)) {
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

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }

  return true;
}

bool DatabasePublisherInfo::InsertOrUpdate(
    sql::Database* db,
    ledger::PublisherInfoPtr info) {
  if (!info || info->id.empty()) {
    return false;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_id, excluded, name, url, provider, favIcon) "
      "VALUES (?, ?, ?, ?, ?, "
      "(SELECT IFNULL( "
      "(SELECT favicon FROM %s "
      "WHERE publisher_id = ?), \"\")))",
      table_name_,
      table_name_);

  sql::Statement statement(
      db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statement.BindString(0, info->id);
  statement.BindInt(1, static_cast<int>(info->excluded));
  statement.BindString(2, info->name);
  statement.BindString(3, info->url);
  statement.BindString(4, info->provider);
  statement.BindString(5, info->id);

  if (!statement.Run()) {
    return false;
  }

  std::string favicon = info->favicon_url;
  if (!favicon.empty()) {
    const std::string query_icon = base::StringPrintf(
        "UPDATE %s SET favIcon = ? WHERE publisher_id = ?",
        table_name_);

    sql::Statement statement_icon(
      db->GetCachedStatement(SQL_FROM_HERE, query_icon.c_str()));

    if (favicon == ledger::kClearFavicon) {
      favicon.clear();
    }

    statement_icon.BindString(0, favicon);
    statement_icon.BindString(1, info->id);

    statement_icon.Run();
  }

  return transaction.Commit();
}

ledger::PublisherInfoPtr DatabasePublisherInfo::GetRecord(
    sql::Database* db,
    const std::string& publisher_key) {
  if (publisher_key.empty()) {
    return nullptr;
  }

  const std::string query = base::StringPrintf(
    "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, pi.provider, "
    "spi.status, pi.excluded "
    "FROM %s as pi "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = pi.publisher_id "
    "WHERE publisher_id=?",
    table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));
  statement.BindString(0, publisher_key);

  if (!statement.Step()) {
    return nullptr;
  }

  auto info = ledger::PublisherInfo::New();
  info->id = statement.ColumnString(0);
  info->name = statement.ColumnString(1);
  info->url = statement.ColumnString(2);
  info->favicon_url = statement.ColumnString(3);
  info->provider = statement.ColumnString(4);
  info->status =
      static_cast<ledger::mojom::PublisherStatus>(statement.ColumnInt64(5));
  info->excluded = static_cast<ledger::PublisherExclude>(
      statement.ColumnInt(6));

  return info;
}

ledger::PublisherInfoPtr DatabasePublisherInfo::GetPanelRecord(
    sql::Database* db,
    ledger::ActivityInfoFilterPtr filter) {
  if (!filter || filter->id.empty()) {
    return nullptr;
  }

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
    table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));
  statement.BindString(0, filter->id);
  statement.BindInt64(1, filter->reconcile_stamp);
  statement.BindString(2, filter->id);

  if (!statement.Step()) {
    return nullptr;
  }

  auto info = ledger::PublisherInfo::New();
  info->id = statement.ColumnString(0);
  info->name = statement.ColumnString(1);
  info->url = statement.ColumnString(2);
  info->favicon_url = statement.ColumnString(3);
  info->provider = statement.ColumnString(4);
  info->status =
      static_cast<ledger::mojom::PublisherStatus>(statement.ColumnInt64(5));
  info->excluded = static_cast<ledger::PublisherExclude>(
      statement.ColumnInt(6));
  info->percent = statement.ColumnInt(7);

  return info;
}

bool DatabasePublisherInfo::RestorePublishers(sql::Database* db) {
  const std::string query = base::StringPrintf(
    "UPDATE %s SET excluded=? WHERE excluded=?",
    table_name_);

  sql::Statement statement(
      db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));
  statement.BindInt(0, static_cast<int>(ledger::PublisherExclude::DEFAULT));
  statement.BindInt(1, static_cast<int>(ledger::PublisherExclude::EXCLUDED));

  return statement.Run();
}

bool DatabasePublisherInfo::GetExcludedList(
    sql::Database* db,
    ledger::PublisherInfoList* list) {
  DCHECK(list);
  if (!list) {
    return false;
  }

  const std::string query = base::StringPrintf(
    "SELECT pi.publisher_id, spi.status, pi.name,"
    "pi.favicon, pi.url, pi.provider "
    "FROM %s as pi "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = pi.publisher_id "
    "WHERE pi.excluded = 1",
    table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

  while (statement.Step()) {
    auto info = ledger::PublisherInfo::New();
    info->id = statement.ColumnString(0);
    info->status =
        static_cast<ledger::mojom::PublisherStatus>(statement.ColumnInt64(1));
    info->name = statement.ColumnString(2);
    info->favicon_url = statement.ColumnString(3);
    info->url = statement.ColumnString(4);
    info->provider = statement.ColumnString(5);

    list->push_back(std::move(info));
  }

  return true;
}

}  // namespace brave_rewards
