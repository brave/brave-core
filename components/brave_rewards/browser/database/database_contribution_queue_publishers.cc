/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/database/database_contribution_queue_publishers.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "sql/statement.h"
#include "sql/transaction.h"

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

bool DatabaseContributionQueuePublishers::CreateIndex(sql::Database* db) {
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

}  // namespace brave_rewards
