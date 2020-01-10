/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/browser/database/database_media_publisher_info.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards {

namespace {
  const char* table_name_ = "media_publisher_info";
}  // namespace

DatabaseMediaPublisherInfo::DatabaseMediaPublisherInfo(
    int current_db_version) :
    DatabaseTable(current_db_version) {
}

DatabaseMediaPublisherInfo::~DatabaseMediaPublisherInfo() = default;

bool DatabaseMediaPublisherInfo::CreateTableV1(sql::Database* db) {
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

  return db->Execute(query.c_str());
}

bool DatabaseMediaPublisherInfo::CreateTableV15(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "media_key TEXT NOT NULL PRIMARY KEY UNIQUE,"
        "publisher_id LONGVARCHAR NOT NULL"
      ")",
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseMediaPublisherInfo::CreateIndexV15(sql::Database* db) {
  bool success = this->InsertIndex(db, table_name_, "media_key");

  if (!success) {
    return false;
  }

  return this->InsertIndex(db, table_name_, "publisher_id");
}

bool DatabaseMediaPublisherInfo::Migrate(
    sql::Database* db,
    const int target) {
  switch (target) {
    case 1: {
      return MigrateToV1(db);
    }
    case 15: {
      return MigrateToV15(db);
    }
    default: {
      NOTREACHED();
      return false;
    }
  }
}

bool DatabaseMediaPublisherInfo::MigrateToV1(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    DropTable(db, table_name_);
  }

  if (!CreateTableV1(db)) {
    return false;
  }

  return true;
}

bool DatabaseMediaPublisherInfo::MigrateToV15(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  if (!CreateTableV15(db)) {
    return false;
  }

  if (!CreateIndexV15(db)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "media_key", "media_key" },
    { "publisher_id", "publisher_id" }
  };

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }

  return true;
}

bool DatabaseMediaPublisherInfo::InsertOrUpdate(
    sql::Database* db,
    const std::string& media_key,
    const std::string& publisher_key) {
  if (media_key.empty() || publisher_key.empty()) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s (media_key, publisher_id) VALUES (?, ?)",
      table_name_);

  sql::Statement statement(
      db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statement.BindString(0, media_key);
  statement.BindString(1, publisher_key);

  return statement.Run();
}

ledger::PublisherInfoPtr DatabaseMediaPublisherInfo::GetRecord(
    sql::Database* db,
    const std::string& media_key) {
  if (media_key.empty()) {
    return nullptr;
  }

  const std::string query = base::StringPrintf(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
      "pi.provider, spi.status, pi.excluded "
      "FROM %s as mpi "
      "INNER JOIN publisher_info AS pi ON mpi.publisher_id = pi.publisher_id "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE mpi.media_key=?",
      table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

  statement.BindString(0, media_key);

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

}  // namespace brave_rewards
