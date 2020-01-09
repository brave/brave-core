/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/browser/database/database_activity_info.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards {

namespace {
  const char* table_name_ = "activity_info";
  const int minimum_version_ = 1;
}  // namespace

std::string GenerateActivityFilterQuery(
    const int start,
    const int limit,
    ledger::ActivityInfoFilterPtr filter) {
  std::string query = "";
  if (!filter) {
    return query;
  }

  if (!filter->id.empty()) {
    query += " AND ai.publisher_id = ?";
  }

  if (filter->reconcile_stamp > 0) {
    query += " AND ai.reconcile_stamp = ?";
  }

  if (filter->min_duration > 0) {
    query += " AND ai.duration >= ?";
  }

  if (filter->excluded != ledger::ExcludeFilter::FILTER_ALL &&
      filter->excluded !=
        ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    query += " AND pi.excluded = ?";
  }

  if (filter->excluded ==
    ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    query += " AND pi.excluded != ?";
  }

  if (filter->percent > 0) {
    query += " AND ai.percent >= ?";
  }

  if (filter->min_visits > 0) {
    query += " AND ai.visits >= ?";
  }

  if (!filter->non_verified) {
    const std::string status = base::StringPrintf(
        " AND spi.status != %1d",
        ledger::mojom::PublisherStatus::NOT_VERIFIED);
    query += status;
  }

  for (const auto& it : filter->order_by) {
    query += " ORDER BY " + it->property_name;
    query += (it->ascending ? " ASC" : " DESC");
  }

  if (limit > 0) {
    query += " LIMIT " + std::to_string(limit);

    if (start > 1) {
      query += " OFFSET " + std::to_string(start);
    }
  }

  return query;
}

void GenerateActivityFilterBind(
    sql::Statement* statement,
    ledger::ActivityInfoFilterPtr filter) {
  if (!statement || !filter) {
    return;
  }

  int column = 0;
  if (!filter->id.empty()) {
    statement->BindString(column++, filter->id);
  }

  if (filter->reconcile_stamp > 0) {
    statement->BindInt64(column++, filter->reconcile_stamp);
  }

  if (filter->min_duration > 0) {
    statement->BindInt(column++, filter->min_duration);
  }

  if (filter->excluded != ledger::ExcludeFilter::FILTER_ALL &&
      filter->excluded !=
      ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    statement->BindInt(column++, static_cast<int32_t>(filter->excluded));
  }

  if (filter->excluded ==
      ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    statement->BindInt(column++,
        static_cast<int>(ledger::PublisherExclude::EXCLUDED));
  }

  if (filter->percent > 0) {
    statement->BindInt(column++, filter->percent);
  }

  if (filter->min_visits > 0) {
    statement->BindInt(column++, filter->min_visits);
  }
}

DatabaseActivityInfo::DatabaseActivityInfo(
    int current_db_version) :
    DatabaseTable(current_db_version) {
}

DatabaseActivityInfo::~DatabaseActivityInfo() = default;

bool DatabaseActivityInfo::Init(sql::Database* db) {
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

bool DatabaseActivityInfo::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  return CreateTableV15(db);
}

bool DatabaseActivityInfo::CreateTableV1(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL,"
        "duration INTEGER DEFAULT 0 NOT NULL,"
        "score DOUBLE DEFAULT 0 NOT NULL,"
        "percent INTEGER DEFAULT 0 NOT NULL,"
        "weight DOUBLE DEFAULT 0 NOT NULL,"
        "category INTEGER NOT NULL,"
        "month INTEGER NOT NULL,"
        "year INTEGER NOT NULL,"
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      table_name_,
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseActivityInfo::CreateTableV2(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL,"
        "duration INTEGER DEFAULT 0 NOT NULL,"
        "score DOUBLE DEFAULT 0 NOT NULL,"
        "percent INTEGER DEFAULT 0 NOT NULL,"
        "weight DOUBLE DEFAULT 0 NOT NULL,"
        "category INTEGER NOT NULL,"
        "month INTEGER NOT NULL,"
        "year INTEGER NOT NULL,"
        "reconcile_stamp INTEGER DEFAULT 0 NOT NULL,"
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      table_name_,
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseActivityInfo::CreateTableV4(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL,"
        "duration INTEGER DEFAULT 0 NOT NULL,"
        "visits INTEGER DEFAULT 0 NOT NULL,"
        "score DOUBLE DEFAULT 0 NOT NULL,"
        "percent INTEGER DEFAULT 0 NOT NULL,"
        "weight DOUBLE DEFAULT 0 NOT NULL,"
        "month INTEGER NOT NULL,"
        "year INTEGER NOT NULL,"
        "reconcile_stamp INTEGER DEFAULT 0 NOT NULL,"
        "CONSTRAINT activity_unique "
        "UNIQUE (publisher_id, month, year, reconcile_stamp) "
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      table_name_,
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseActivityInfo::CreateTableV6(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL,"
        "duration INTEGER DEFAULT 0 NOT NULL,"
        "visits INTEGER DEFAULT 0 NOT NULL,"
        "score DOUBLE DEFAULT 0 NOT NULL,"
        "percent INTEGER DEFAULT 0 NOT NULL,"
        "weight DOUBLE DEFAULT 0 NOT NULL,"
        "reconcile_stamp INTEGER DEFAULT 0 NOT NULL,"
        "CONSTRAINT activity_unique "
        "UNIQUE (publisher_id, reconcile_stamp) "
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      table_name_,
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseActivityInfo::CreateTableV15(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL,"
        "duration INTEGER DEFAULT 0 NOT NULL,"
        "visits INTEGER DEFAULT 0 NOT NULL,"
        "score DOUBLE DEFAULT 0 NOT NULL,"
        "percent INTEGER DEFAULT 0 NOT NULL,"
        "weight DOUBLE DEFAULT 0 NOT NULL,"
        "reconcile_stamp INTEGER DEFAULT 0 NOT NULL,"
        "CONSTRAINT activity_unique "
        "UNIQUE (publisher_id, reconcile_stamp)"
      ")",
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseActivityInfo::CreateIndex(sql::Database* db) {
  return CreateIndexV15(db);
}

bool DatabaseActivityInfo::CreateIndexV2(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_id");
}

bool DatabaseActivityInfo::CreateIndexV4(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_id");
}

bool DatabaseActivityInfo::CreateIndexV6(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_id");
}

bool DatabaseActivityInfo::CreateIndexV15(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_id");
}

bool DatabaseActivityInfo::Migrate(sql::Database* db, const int target) {
  switch (target) {
    case 2: {
      return MigrateToV2(db);
    }
    case 4: {
      return MigrateToV4(db);
    }
    case 5: {
      return MigrateToV5(db);
    }
    case 6: {
      return MigrateToV6(db);
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

bool DatabaseActivityInfo::MigrateToV2(sql::Database* db) {
  if (!db->DoesTableExist(table_name_)) {
    if (!CreateTableV2(db)) {
      return false;
    }
  }

  const char* column = "reconcile_stamp";
  if (!db->DoesColumnExist(table_name_, column)) {
    const std::string query = base::StringPrintf(
        "ALTER TABLE %s ADD %s INTEGER DEFAULT 0 NOT NULL;",
        table_name_,
        column);

    sql::Statement statement(
        db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

    return statement.Run();
  }

  return true;
}

bool DatabaseActivityInfo::MigrateToV4(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  std::string sql = "DROP INDEX IF EXISTS activity_info_publisher_id_index;";
  if (!db->Execute(sql.c_str())) {
    return false;
  }

  if (!CreateTableV4(db)) {
    return false;
  }

  if (!CreateIndexV4(db)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "publisher_id", "publisher_id" },
    { "duration", "duration" },
    { "score", "score" },
    { "percent", "percent" },
    { "weight", "weight" },
    { "month", "month" },
    { "year", "year" },
    { "reconcile_stamp", "reconcile_stamp" }
  };

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }

  sql = base::StringPrintf("UPDATE %s SET visits=5;", table_name_);
  if (!db->Execute(sql.c_str())) {
    return false;
  }

  return true;
}

bool DatabaseActivityInfo::MigrateToV5(sql::Database* db) {
  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  const std::string query_update = base::StringPrintf(
      "UPDATE %s SET visits = 1 "
      "WHERE publisher_id = ? AND month = ? AND "
      "year = ? AND reconcile_stamp = ?",
      table_name_);

  const std::string query_select = base::StringPrintf(
      "SELECT publisher_id, month, year, reconcile_stamp "
      "FROM %s WHERE visits = 0",
      table_name_);

  sql::Statement statement_select(
      db->GetCachedStatement(SQL_FROM_HERE, query_select.c_str()));

  while (statement_select.Step()) {
    sql::Statement statement_update(
        db->GetCachedStatement(SQL_FROM_HERE, query_update.c_str()));

    statement_update.BindString(0, statement_select.ColumnString(0));
    statement_update.BindInt(1, statement_select.ColumnInt(1));
    statement_update.BindInt(2, statement_select.ColumnInt(2));
    statement_update.BindInt64(3, statement_select.ColumnInt64(3));
    statement_update.Run();
  }

  return transaction.Commit();
}

bool DatabaseActivityInfo::MigrateToV6(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  const std::string sql =
      "DROP INDEX IF EXISTS activity_info_publisher_id_index;";
  if (!db->Execute(sql.c_str())) {
    return false;
  }

  if (!CreateTableV6(db)) {
    return false;
  }

  if (!CreateIndexV6(db)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "publisher_id", "publisher_id" },
    { "sum(duration) as duration", "duration" },
    { "sum(visits) as visits", "visits" },
    { "sum(score) as score", "score" },
    { "sum(percent) as percent", "percent" },
    { "sum(weight) as weight", "weight" },
    { "reconcile_stamp", "reconcile_stamp" }
  };

  const std::string group_by = "GROUP BY publisher_id, reconcile_stamp";

  if (!MigrateDBTable(
      db,
      temp_table_name,
      table_name_,
      columns,
      true,
      group_by)) {
    return false;
  }

  return true;
}

bool DatabaseActivityInfo::MigrateToV15(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  const std::string sql =
      "DROP INDEX IF EXISTS activity_info_publisher_id_index;";
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
    { "duration", "duration" },
    { "visits", "visits" },
    { "score", "score" },
    { "percent", "percent" },
    { "weight", "weight" },
    { "reconcile_stamp", "reconcile_stamp" }
  };

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }

  return true;
}

bool DatabaseActivityInfo::InsertOrUpdate(
    sql::Database* db,
    ledger::PublisherInfoPtr info) {
  if (!info) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_id, duration, score, percent, "
      "weight, reconcile_stamp, visits) "
      "VALUES (?, ?, ?, ?, ?, ?, ?)",
      table_name_);

  sql::Statement statement(
      db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statement.BindString(0, info->id);
  statement.BindInt64(1, static_cast<int>(info->duration));
  statement.BindDouble(2, info->score);
  statement.BindInt64(3, static_cast<int>(info->percent));
  statement.BindDouble(4, info->weight);
  statement.BindInt64(5, info->reconcile_stamp);
  statement.BindInt(6, info->visits);

  return statement.Run();
}

bool DatabaseActivityInfo::GetRecordsList(
    sql::Database* db,
    const int start,
    const int limit,
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoList* list) {
  DCHECK(list);
  if (!list || !filter) {
    return false;
  }

  std::string query = base::StringPrintf(
    "SELECT ai.publisher_id, ai.duration, ai.score, "
    "ai.percent, ai.weight, spi.status, pi.excluded, "
    "pi.name, pi.url, pi.provider, "
    "pi.favIcon, ai.reconcile_stamp, ai.visits "
    "FROM %s AS ai "
    "INNER JOIN publisher_info AS pi "
    "ON ai.publisher_id = pi.publisher_id "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = pi.publisher_id "
    "WHERE 1 = 1",
    table_name_);

  query += GenerateActivityFilterQuery(start, limit, filter->Clone());

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

  GenerateActivityFilterBind(&statement, filter->Clone());

  while (statement.Step()) {
    auto info = ledger::PublisherInfo::New();
    info->id = statement.ColumnString(0);
    info->duration = statement.ColumnInt64(1);
    info->score = statement.ColumnDouble(2);
    info->percent = statement.ColumnInt64(3);
    info->weight = statement.ColumnDouble(4);
    info->status =
        static_cast<ledger::mojom::PublisherStatus>(statement.ColumnInt64(5));
    info->excluded = static_cast<ledger::PublisherExclude>(
        statement.ColumnInt(6));
    info->name = statement.ColumnString(7);
    info->url = statement.ColumnString(8);
    info->provider = statement.ColumnString(9);
    info->favicon_url = statement.ColumnString(10);
    info->reconcile_stamp = statement.ColumnInt64(11);
    info->visits = statement.ColumnInt(12);

    list->push_back(std::move(info));
  }

  return true;
}

bool DatabaseActivityInfo::DeleteRecord(
    sql::Database* db,
    const std::string& publisher_key,
    const uint64_t reconcile_stamp) {
  if (publisher_key.empty() || reconcile_stamp == 0) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "DELETE FROM %s WHERE publisher_id = ? AND reconcile_stamp = ?",
      table_name_);

  sql::Statement statement(
      db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statement.BindString(0, publisher_key);
  statement.BindInt64(1, reconcile_stamp);

  return statement.Run();
}

}  // namespace brave_rewards
