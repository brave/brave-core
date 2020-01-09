/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/browser/database/database_recurring_tip.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards {

namespace {
  // TODO(https://github.com/brave/brave-browser/issues/7144):
  //  rename to recurring_tip
  const char* table_name_ = "recurring_donation";
  const int minimum_version_ = 2;
}  // namespace

DatabaseRecurringTip::DatabaseRecurringTip(
    int current_db_version) :
    DatabaseTable(current_db_version) {
}

DatabaseRecurringTip::~DatabaseRecurringTip() = default;

bool DatabaseRecurringTip::Init(sql::Database* db) {
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

bool DatabaseRecurringTip::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  return CreateTableV15(db);
}

bool DatabaseRecurringTip::CreateTableV2(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL PRIMARY KEY UNIQUE,"
        "amount DOUBLE DEFAULT 0 NOT NULL,"
        "added_date INTEGER DEFAULT 0 NOT NULL,"
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      table_name_,
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseRecurringTip::CreateTableV15(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL PRIMARY KEY UNIQUE,"
        "amount DOUBLE DEFAULT 0 NOT NULL,"
        "added_date INTEGER DEFAULT 0 NOT NULL"
      ")",
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseRecurringTip::CreateIndex(sql::Database* db) {
  return CreateIndexV15(db);
}

bool DatabaseRecurringTip::CreateIndexV2(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_id");
}

bool DatabaseRecurringTip::CreateIndexV15(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_id");
}

bool DatabaseRecurringTip::Migrate(sql::Database* db, const int target) {
  switch (target) {
    case 2: {
      return MigrateToV2(db);
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

bool DatabaseRecurringTip::MigrateToV2(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    DropTable(db, table_name_);
  }

  if (!CreateTableV2(db)) {
    return false;
  }

  if (!CreateIndexV2(db)) {
    return false;
  }

  return true;
}

bool DatabaseRecurringTip::MigrateToV15(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  const std::string sql =
      "DROP INDEX IF EXISTS recurring_donation_publisher_id_index;";
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
    { "publisher_id", "publisher_id" },
    { "amount", "amount" },
    { "added_date", "added_date" }
  };

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }
  return true;
}

bool DatabaseRecurringTip::InsertOrUpdate(
    sql::Database* db,
    ledger::RecurringTipPtr info) {
  if (!info || info->publisher_key.empty()) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_id, amount, added_date) "
      "VALUES (?, ?, ?)",
      table_name_);

  sql::Statement statement(
      db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statement.BindString(0, info->publisher_key);
  statement.BindDouble(1, info->amount);
  statement.BindInt64(2, info->created_at);

  return statement.Run();
}
void DatabaseRecurringTip::GetAllRecords(
    sql::Database* db,
    ledger::PublisherInfoList* list) {
  DCHECK(list);
  if (!list) {
    return;
  }

  const std::string query = base::StringPrintf(
    "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
    "rd.amount, rd.added_date, spi.status, pi.provider "
    "FROM %s as rd "
    "INNER JOIN publisher_info AS pi ON rd.publisher_id = pi.publisher_id "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = pi.publisher_id ",
    table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

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
}

bool DatabaseRecurringTip::DeleteRecord(
    sql::Database* db,
    const std::string& publisher_key) {
  const std::string query = base::StringPrintf(
    "DELETE FROM %s WHERE publisher_id = ?",
    table_name_);

  sql::Statement statement(
      db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statement.BindString(0, publisher_key);

  return statement.Run();
}

}  // namespace brave_rewards
