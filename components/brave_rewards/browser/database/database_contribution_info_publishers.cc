/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/browser/database/database_contribution_info_publishers.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {
  const char table_name_[] = "contribution_info_publishers";
}  // namespace

namespace brave_rewards {

DatabaseContributionInfoPublishers::DatabaseContributionInfoPublishers
    (int current_db_version) : DatabaseTable(current_db_version) {
}

DatabaseContributionInfoPublishers::
~DatabaseContributionInfoPublishers()= default;

bool DatabaseContributionInfoPublishers::CreateTableV11(sql::Database* db) {
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

  return db->Execute(query.c_str());
}

bool DatabaseContributionInfoPublishers::CreateTableV15(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "contribution_id TEXT NOT NULL,"
        "publisher_key TEXT NOT NULL,"
        "total_amount DOUBLE NOT NULL,"
        "contributed_amount DOUBLE"
      ")",
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseContributionInfoPublishers::CreateIndexV11(sql::Database* db) {
  bool success = this->InsertIndex(db, table_name_, "contribution_id");

  if (!success) {
    return false;
  }

  return this->InsertIndex(db, table_name_, "publisher_key");
}

bool DatabaseContributionInfoPublishers::CreateIndexV15(sql::Database* db) {
  bool success = this->InsertIndex(db, table_name_, "contribution_id");

  if (!success) {
    return false;
  }

  return this->InsertIndex(db, table_name_, "publisher_key");
}

bool DatabaseContributionInfoPublishers::Migrate(
    sql::Database* db,
    const int target) {
  switch (target) {
    case 11: {
      return MigrateToV11(db);
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

bool DatabaseContributionInfoPublishers::MigrateToV11(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    DropTable(db, table_name_);
  }

  if (!CreateTableV11(db)) {
    return false;
  }

  if (!CreateIndexV11(db)) {
    return false;
  }

  return true;
}

bool DatabaseContributionInfoPublishers::MigrateToV15(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  const std::string sql =
      "DROP INDEX IF EXISTS contribution_info_publishers_contribution_id_index;"
      " DROP INDEX IF EXISTS contribution_info_publishers_publisher_key_index;";
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
    { "contribution_id", "contribution_id" },
    { "publisher_key", "publisher_key" },
    { "total_amount", "total_amount" },
    { "contributed_amount", "contributed_amount" }
  };

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }

  return true;
}

bool DatabaseContributionInfoPublishers::InsertOrUpdate(
    sql::Database* db,
    ledger::ContributionInfoPtr info) {
  if (!info) {
    return false;
  }

  const std::string query = base::StringPrintf(
    "INSERT OR REPLACE INTO %s "
    "(contribution_id, publisher_key, total_amount, contributed_amount) "
    "VALUES (?, ?, ?, ?)",
    table_name_);

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  for (const auto& publisher : info->publishers) {
    sql::Statement statement(
      db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

    statement.BindString(0, publisher->contribution_id);
    statement.BindString(1, publisher->publisher_key);
    statement.BindDouble(2, publisher->total_amount);
    statement.BindDouble(3, publisher->contributed_amount);
    statement.Run();
  }

  return transaction.Commit();
}

bool DatabaseContributionInfoPublishers::GetRecords(
    sql::Database* db,
    const std::string& contribution_id,
    ledger::ContributionPublisherList* list) {
  if (contribution_id.empty() || !list) {
    return false;
  }

  const std::string query = base::StringPrintf(
    "SELECT contribution_id, publisher_key, total_amount, contributed_amount "
    "FROM %s WHERE contribution_id = ?",
    table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

  statement.BindString(0, contribution_id);

  while (statement.Step()) {
    auto publisher = ledger::ContributionPublisher::New();

    publisher->contribution_id = statement.ColumnString(0);
    publisher->publisher_key = statement.ColumnString(1);
    publisher->total_amount = statement.ColumnDouble(2);
    publisher->contributed_amount = statement.ColumnDouble(3);

    list->push_back(std::move(publisher));
  }

  return true;
}

bool DatabaseContributionInfoPublishers::GetPublisherInfoList(
    sql::Database* db,
    const std::string& contribution_id,
    ledger::PublisherInfoList* list) {
  DCHECK(list && db);
  if (contribution_id.empty() || !list) {
    return false;
  }

  const std::string query = base::StringPrintf(
    "SELECT cip.publisher_key, cip.total_amount, pi.name, pi.url, pi.favIcon, "
    "spi.status, pi.provider FROM %s as cip "
    "INNER JOIN publisher_info AS pi ON cip.publisher_key = pi.publisher_id "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = cip.publisher_key "
    "WHERE cip.contribution_id = ?",
    table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));
  statement.BindString(0, contribution_id);

  while (statement.Step()) {
    auto publisher = ledger::PublisherInfo::New();

    publisher->id = statement.ColumnString(0);
    publisher->weight = statement.ColumnDouble(1);
    publisher->name = statement.ColumnString(2);
    publisher->url = statement.ColumnString(3);
    publisher->favicon_url = statement.ColumnString(4);
    publisher->status =
        static_cast<ledger::mojom::PublisherStatus>(statement.ColumnInt64(5));
    publisher->provider = statement.ColumnString(6);

    list->push_back(std::move(publisher));
  }

  return true;
}

}  // namespace brave_rewards
