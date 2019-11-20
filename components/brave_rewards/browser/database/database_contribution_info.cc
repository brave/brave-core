/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/browser/database/database_contribution_info.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards {

DatabaseContributionInfo::DatabaseContributionInfo(int current_db_version) :
    DatabaseTable(current_db_version) {
}

DatabaseContributionInfo::~DatabaseContributionInfo() {
}

bool DatabaseContributionInfo::Init(sql::Database* db) {
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

bool DatabaseContributionInfo::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  return CreateTableV8(db);
}

bool DatabaseContributionInfo::CreateIndex(sql::Database* db) {
  return CreateIndexV8(db);
}

bool DatabaseContributionInfo::CreateTableV8(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR,"
        "probi TEXT \"0\"  NOT NULL,"
        "date INTEGER NOT NULL,"
        "type INTEGER NOT NULL,"
        "month INTEGER NOT NULL,"
        "year INTEGER NOT NULL,"
        "CONSTRAINT fk_contribution_info_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseContributionInfo::CreateIndexV8(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_id");
}

bool DatabaseContributionInfo::Migrate(sql::Database* db, const int to) {
  switch (to) {
    case 8: {
      return MigrateToV8(db);
    }
    default: {
      NOTREACHED();
      return false;
    }
  }
}

bool DatabaseContributionInfo::MigrateToV8(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  const std::string sql =
      "DROP INDEX IF EXISTS contribution_info_publisher_id_index;";
  if (!db->Execute(sql.c_str())) {
    return false;
  }

  if (!CreateTableV8(db)) {
    return false;
  }

  if (!CreateIndexV8(db)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "publisher_id", "publisher_id" },
    { "probi", "probi" },
    { "date", "date" },
    { "category", "type" },
    { "month", "month" },
    { "year", "year" }
  };

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }

  return true;
}

bool DatabaseContributionInfo::InsertOrUpdate(
    sql::Database* db,
    const brave_rewards::ContributionInfo& info) {
  const std::string query = base::StringPrintf(
    "INSERT OR REPLACE INTO %s "
    "(publisher_id, probi, date, type, month, year) "
    "VALUES (?, ?, ?, ?, ?, ?)",
    table_name_);

  sql::Statement statement(
      db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statement.BindString(0, info.publisher_key);
  statement.BindString(1, info.probi);
  statement.BindInt64(2, info.date);
  statement.BindInt(3, info.type);
  statement.BindInt(4, info.month);
  statement.BindInt(5, info.year);


  return statement.Run();
}

bool DatabaseContributionInfo::GetOneTimeTips(
    sql::Database* db,
    ledger::PublisherInfoList* list,
    const ledger::ActivityMonth month,
    const int year) {
  if (!list) {
    return false;
  }

  const std::string query = base::StringPrintf(
    "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
    "ci.probi, ci.date, spi.status, pi.provider "
    "FROM %s as ci "
    "INNER JOIN publisher_info AS pi ON ci.publisher_id = pi.publisher_id "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = pi.publisher_id "
    "WHERE ci.month = ? AND ci.year = ? AND ci.type = ?",
    table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

  statement.BindInt(0, static_cast<int>(month));
  statement.BindInt(1, year);
  statement.BindInt(2, static_cast<int>(ledger::RewardsType::ONE_TIME_TIP));

  while (statement.Step()) {
    auto publisher = ledger::PublisherInfo::New();

    publisher->id = statement.ColumnString(0);
    publisher->name = statement.ColumnString(1);
    publisher->url = statement.ColumnString(2);
    publisher->favicon_url = statement.ColumnString(3);
    publisher->weight = statement.ColumnDouble(4);
    publisher->reconcile_stamp = statement.ColumnInt64(5);
    publisher->status =
        static_cast<ledger::mojom::PublisherStatus>(statement.ColumnInt64(6));
    publisher->provider = statement.ColumnString(7);

    list->push_back(std::move(publisher));
  }

  return true;
}

}  // namespace brave_rewards
