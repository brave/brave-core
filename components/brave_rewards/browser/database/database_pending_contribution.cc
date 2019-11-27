/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_rewards/browser/database/database_pending_contribution.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards {

namespace {
  const char* table_name_ = "pending_contribution";
  const int minimum_version_ = 3;
}  // namespace

DatabasePendingContribution::DatabasePendingContribution(
    int current_db_version) :
    DatabaseTable(current_db_version) {
}

DatabasePendingContribution::~DatabasePendingContribution() = default;

bool DatabasePendingContribution::Init(sql::Database* db) {
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

bool DatabasePendingContribution::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  return CreateTableV8(db);
}

bool DatabasePendingContribution::CreateTableV3(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL,"
        "amount DOUBLE DEFAULT 0 NOT NULL,"
        "added_date INTEGER DEFAULT 0 NOT NULL,"
        "viewing_id LONGVARCHAR NOT NULL,"
        "category INTEGER NOT NULL,"
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      table_name_,
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabasePendingContribution::CreateTableV8(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR NOT NULL,"
        "amount DOUBLE DEFAULT 0 NOT NULL,"
        "added_date INTEGER DEFAULT 0 NOT NULL,"
        "viewing_id LONGVARCHAR NOT NULL,"
        "type INTEGER NOT NULL,"
        "CONSTRAINT fk_%s_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      table_name_,
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabasePendingContribution::CreateIndex(sql::Database* db) {
  return CreateIndexV8(db);
}

bool DatabasePendingContribution::CreateIndexV3(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_id");
}

bool DatabasePendingContribution::CreateIndexV8(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_id");
}

bool DatabasePendingContribution::Migrate(sql::Database* db, const int target) {
  switch (target) {
    case 3: {
      return MigrateToV3(db);
    }
    case 8: {
      return MigrateToV8(db);
    }
    default: {
      NOTREACHED();
      return false;
    }
  }
}

bool DatabasePendingContribution::MigrateToV3(sql::Database* db) {
  if (!CreateTableV3(db)) {
    return false;
  }

  if (!CreateIndexV3(db)) {
    return false;
  }

  return true;
}

bool DatabasePendingContribution::MigrateToV8(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  const std::string sql =
      "DROP INDEX IF EXISTS pending_contribution_publisher_id_index;";
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
    { "amount", "amount" },
    { "added_date", "added_date" },
    { "viewing_id", "viewing_id" },
    { "category", "type" }
  };

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }

  return true;
}

bool DatabasePendingContribution::InsertOrUpdate(
    sql::Database* db,
    ledger::PendingContributionList list) {
  if (list.size() == 0) {
    return false;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  const uint64_t now = static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  const std::string query = base::StringPrintf(
    "INSERT INTO %s (publisher_id, amount, added_date, viewing_id, type) "
    "VALUES (?, ?, ?, ?, ?)",
    table_name_);

  for (const auto& item : list) {
    sql::Statement statement(
        db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

    statement.BindString(0, item->publisher_key);
    statement.BindDouble(1, item->amount);
    statement.BindInt64(2, now);
    statement.BindString(3, item->viewing_id);
    statement.BindInt(4, static_cast<int>(item->type));
    if (!statement.Run()) {
      return false;
    }
  }

  return transaction.Commit();
}

double DatabasePendingContribution::GetReservedAmount(sql::Database *db) {
  const std::string query = base::StringPrintf(
    "SELECT SUM(amount) FROM %s",
    table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

  double amount = 0.0;

  if (statement.Step()) {
    amount = statement.ColumnDouble(0);
  }

  return amount;
}

void DatabasePendingContribution::GetAllRecords(
    sql::Database* db,
    ledger::PendingContributionInfoList* list) {
  if (!list) {
    return;
  }

  const std::string query = base::StringPrintf(
    "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
    "spi.status, pi.provider, pc.amount, pc.added_date, "
    "pc.viewing_id, pc.type "
    "FROM %s as pc "
    "INNER JOIN publisher_info AS pi ON pc.publisher_id = pi.publisher_id "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = pi.publisher_id",
    table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

  while (statement.Step()) {
    auto info = ledger::PendingContributionInfo::New();
    info->publisher_key = statement.ColumnString(0);
    info->name = statement.ColumnString(1);
    info->url = statement.ColumnString(2);
    info->favicon_url = statement.ColumnString(3);
    info->status =
        static_cast<ledger::mojom::PublisherStatus>(statement.ColumnInt64(4));
    info->provider = statement.ColumnString(5);
    info->amount = statement.ColumnDouble(6);
    info->added_date = statement.ColumnInt64(7);
    info->viewing_id = statement.ColumnString(8);
    info->type =
        static_cast<ledger::RewardsType>(statement.ColumnInt(9));

    list->push_back(std::move(info));
  }
}

bool DatabasePendingContribution::DeleteRecord(
    sql::Database* db,
    const std::string& publisher_key,
    const std::string& viewing_id,
    uint64_t added_date) {
  const std::string query = base::StringPrintf(
    "DELETE FROM %s "
    "WHERE publisher_id = ? AND viewing_id=? AND added_date=?",
    table_name_);

  sql::Statement statement(
      db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statement.BindString(0, publisher_key);
  statement.BindString(1, viewing_id);
  statement.BindInt64(2, added_date);

  return statement.Run();
}

bool DatabasePendingContribution::DeleteAllRecords(sql::Database* db) {
  const std::string query = base::StringPrintf(
    "DELETE FROM %s",
    table_name_);

  sql::Statement statement(
      db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));
  return statement.Run();
}

}  // namespace brave_rewards
