/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/browser/database/database_server_publisher_amounts.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {
  const char table_name_[] = "server_publisher_amounts";
  const int minimum_version_ = 7;
}  // namespace

namespace brave_rewards {

DatabaseServerPublisherAmounts::DatabaseServerPublisherAmounts(
    int current_db_version) :
    DatabaseTable(current_db_version) {
}

DatabaseServerPublisherAmounts::~DatabaseServerPublisherAmounts() {
}

bool DatabaseServerPublisherAmounts::Init(sql::Database* db) {
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

bool DatabaseServerPublisherAmounts::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  return CreateTableV15(db);
}

bool DatabaseServerPublisherAmounts::CreateTableV7(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "publisher_key LONGVARCHAR NOT NULL,"
      "amount DOUBLE DEFAULT 0 NOT NULL,"
      "CONSTRAINT %s_unique "
      "    UNIQUE (publisher_key, amount) "
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

bool DatabaseServerPublisherAmounts::CreateTableV15(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "publisher_key LONGVARCHAR NOT NULL,"
      "amount DOUBLE DEFAULT 0 NOT NULL,"
      "CONSTRAINT %s_unique "
      "    UNIQUE (publisher_key, amount)"
      ")",
      table_name_,
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseServerPublisherAmounts::CreateIndex(sql::Database* db) {
  return CreateIndexV15(db);
}

bool DatabaseServerPublisherAmounts::CreateIndexV7(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_key");
}

bool DatabaseServerPublisherAmounts::CreateIndexV15(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_key");
}

bool DatabaseServerPublisherAmounts::Migrate(
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

bool DatabaseServerPublisherAmounts::MigrateToV7(sql::Database* db) {
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

bool DatabaseServerPublisherAmounts::MigrateToV15(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  const std::string sql =
      "DROP INDEX IF EXISTS server_publisher_amounts_publisher_key_index;";
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
    { "amount", "amount" }
  };

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }
  return true;
}

bool DatabaseServerPublisherAmounts::InsertOrUpdate(
    sql::Database* db,
    ledger::ServerPublisherInfoPtr info) {
  if (!info || !info->banner) {
    return false;
  }

  // It's ok if social links are empty
  if (info->banner->amounts.empty()) {
    return true;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  for (const auto& amount : info->banner->amounts) {
    const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_key, amount) "
      "VALUES (?, ?)",
      table_name_);

    sql::Statement statment(
        db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

    statment.BindString(0, info->publisher_key);
    statment.BindDouble(1, amount);
    statment.Run();
  }

  return transaction.Commit();
}

std::vector<double> DatabaseServerPublisherAmounts::GetRecord(
    sql::Database* db,
    const std::string& publisher_key) {
  const std::string query = base::StringPrintf(
      "SELECT amount FROM %s WHERE publisher_key=?",
      table_name_);

  sql::Statement statment(db->GetUniqueStatement(query.c_str()));
  statment.BindString(0, publisher_key);

  std::vector<double> amounts;
  while (statment.Step()) {
    amounts.push_back(statment.ColumnDouble(0));
  }

  return amounts;
}

}  // namespace brave_rewards
