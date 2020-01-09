/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/browser/database/database_contribution_queue_publishers.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {
  const char table_name_[] = "contribution_queue_publishers";
  const int minimum_version_ = 9;
  const char parent_table_name_[] = "contribution_queue";
}  // namespace

namespace brave_rewards {

DatabaseContributionQueuePublishers::DatabaseContributionQueuePublishers(
    int current_db_version) : DatabaseTable(current_db_version) {
}

DatabaseContributionQueuePublishers::~DatabaseContributionQueuePublishers() {
}

bool DatabaseContributionQueuePublishers::Init(sql::Database* db) {
  if (GetCurrentDBVersion() < minimum_version_) {
    return true;
  }

  bool success = CreateTable(db);
  return success;
}

bool DatabaseContributionQueuePublishers::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  return CreateTableV9(db);
}

bool DatabaseContributionQueuePublishers::CreateTableV9(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "%s_id INTEGER NOT NULL,"
      "publisher_key TEXT NOT NULL,"
      "amount_percent DOUBLE NOT NULL,"
      "CONSTRAINT fk_%s_publisher_key "
      "    FOREIGN KEY (publisher_key) "
      "    REFERENCES publisher_info (publisher_id),"
      "CONSTRAINT fk_%s_id "
      "    FOREIGN KEY (%s_id) "
      "    REFERENCES %s (%s_id) "
      "    ON DELETE CASCADE"
      ")",
      table_name_,
      parent_table_name_,
      table_name_,
      table_name_,
      parent_table_name_,
      parent_table_name_,
      parent_table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseContributionQueuePublishers::CreateTableV15(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "%s_id INTEGER NOT NULL,"
      "publisher_key TEXT NOT NULL,"
      "amount_percent DOUBLE NOT NULL"
      ")",
      table_name_,
      parent_table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseContributionQueuePublishers::CreateIndex(sql::Database* db) {
  return CreateIndexV15(db);
}

bool DatabaseContributionQueuePublishers::CreateIndexV15(sql::Database* db) {
  const std::string key = base::StringPrintf("%s_id", parent_table_name_);
  bool success = this->InsertIndex(db, table_name_, key);

  if (!success) {
    return false;
  }

  return this->InsertIndex(db, table_name_, "publisher_key");
}

bool DatabaseContributionQueuePublishers::Migrate(
    sql::Database* db,
    const int target) {
  switch (target) {
    case 9: {
      return MigrateToV9(db);
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

bool DatabaseContributionQueuePublishers::MigrateToV9(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    DropTable(db, table_name_);
  }

  if (!CreateTableV9(db)) {
    return false;
  }

  return true;
}

bool DatabaseContributionQueuePublishers::MigrateToV15(sql::Database* db) {
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

  const std::string key = base::StringPrintf("%s_id", parent_table_name_);
  const std::map<std::string, std::string> columns = {
    { key, key },
    { "publisher_key", "publisher_key" },
    { "amount_percent", "amount_percent" }
  };

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }

  return true;
}

bool DatabaseContributionQueuePublishers::InsertOrUpdate(
    sql::Database* db,
    ledger::ContributionQueuePtr info) {
  if (!info) {
    return false;
  }

  const std::string query = base::StringPrintf(
    "INSERT OR REPLACE INTO %s (%s_id, publisher_key, amount_percent) "
    "VALUES (?, ?, ?)",
    table_name_,
    parent_table_name_);

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  for (const auto& publisher : info->publishers) {
    sql::Statement statement(
      db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

    statement.BindInt64(0, info->id);
    statement.BindString(1, publisher->publisher_key);
    statement.BindDouble(2, publisher->amount_percent);
    statement.Run();
  }

  return transaction.Commit();
}

ledger::ContributionQueuePublisherList
DatabaseContributionQueuePublishers::GetRecords(
    sql::Database* db,
    const uint64_t queue_id) {
  ledger::ContributionQueuePublisherList list;
  const std::string query = base::StringPrintf(
      "SELECT publisher_key, amount_percent "
      "FROM %s WHERE %s_id = ?",
      table_name_,
      parent_table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));
  statement.BindInt64(0, queue_id);

  while (statement.Step()) {
    ledger::ContributionQueuePublisherPtr publisher =
        ledger::ContributionQueuePublisher::New();

    publisher->publisher_key = statement.ColumnString(0);
    publisher->amount_percent = statement.ColumnDouble(1);
    list.push_back(std::move(publisher));
  }

  return list;
}

bool DatabaseContributionQueuePublishers::DeleteRecordsByQueueId(
    sql::Database* db,
    const uint64_t queue_id) {
  DCHECK(db);
  if (queue_id == 0 || !db) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "DELETE FROM %s WHERE %s_id = ?",
      table_name_,
      parent_table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));
  statement.BindInt64(0, queue_id);

  return statement.Run();
}

bool DatabaseContributionQueuePublishers::DeleteAllRecords(sql::Database* db) {
  DCHECK(db);
  if (!db) {
    return false;
  }

  const std::string query = base::StringPrintf("DELETE FROM %s", table_name_);
  sql::Statement statement(db->GetUniqueStatement(query.c_str()));
  return statement.Run();
}

}  // namespace brave_rewards
