/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/browser/database/database_server_publisher_links.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {
  const char table_name_[] = "server_publisher_links";
}  // namespace

namespace brave_rewards {

DatabaseServerPublisherLinks::DatabaseServerPublisherLinks(
    int current_db_version) :
    DatabaseTable(current_db_version) {
}

DatabaseServerPublisherLinks::~DatabaseServerPublisherLinks() {
}

bool DatabaseServerPublisherLinks::CreateTableV7(sql::Database* db) {
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
      table_name_,
      table_name_,
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseServerPublisherLinks::CreateTableV15(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "publisher_key LONGVARCHAR NOT NULL,"
      "provider TEXT,"
      "link TEXT,"
      "CONSTRAINT %s_unique "
      "    UNIQUE (publisher_key, provider)"
      ")",
      table_name_,
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseServerPublisherLinks::CreateIndexV7(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_key");
}

bool DatabaseServerPublisherLinks::CreateIndexV15(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_key");
}

bool DatabaseServerPublisherLinks::Migrate(
    sql::Database* db,
    const int target) {
  switch (target) {
    case 7: {
      return MigrateToV7(db);
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


bool DatabaseServerPublisherLinks::MigrateToV7(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    DropTable(db, table_name_);
  }

  if (!CreateTableV7(db)) {
    return false;
  }

  if (!CreateIndexV7(db)) {
    return false;
  }

  return true;
}

bool DatabaseServerPublisherLinks::MigrateToV15(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  const std::string sql =
      "DROP INDEX IF EXISTS server_publisher_links_publisher_key_index;";
  if (!db->Execute(sql.c_str())) {
    return false;
  }

  if (!CreateTableV15(db)) {
    return false;
  }

  if (!CreateIndexV15(db)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "publisher_key", "publisher_key" },
    { "provider", "provider" },
    { "link", "link" }
  };

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }

  return true;
}

bool DatabaseServerPublisherLinks::InsertOrUpdate(
    sql::Database* db,
    ledger::ServerPublisherInfoPtr info) {
  if (!info || !info->banner) {
    return false;
  }

  // It's ok if links are empty
  if (info->banner->links.empty()) {
    return true;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  for (const auto& link : info->banner->links) {
    if (link.second.empty()) {
      continue;
    }

    const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_key, provider, link) "
      "VALUES (?, ?, ?)",
      table_name_);

    sql::Statement statment(
        db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

    statment.BindString(0, info->publisher_key);
    statment.BindString(1, link.first);
    statment.BindString(2, link.second);
    statment.Run();
  }

  return transaction.Commit();
}

base::flat_map<std::string, std::string>
DatabaseServerPublisherLinks::GetRecord(
    sql::Database* db,
    const std::string& publisher_key) {
  const std::string query = base::StringPrintf(
      "SELECT provider, link FROM %s WHERE publisher_key=?",
      table_name_);

  sql::Statement statment(db->GetUniqueStatement(query.c_str()));
  statment.BindString(0, publisher_key);

  base::flat_map<std::string, std::string> links;
  while (statment.Step()) {
    const auto pair = std::make_pair(
        statment.ColumnString(0),
        statment.ColumnString(1));
    links.insert(pair);
  }

  return links;
}

}  // namespace brave_rewards
