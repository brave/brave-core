/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/browser/database/database_server_publisher_banner.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {
  const char table_name_[] = "server_publisher_banner";
  const int minimum_version_ = 7;
}  // namespace

namespace brave_rewards {

DatabaseServerPublisherBanner::DatabaseServerPublisherBanner(
    int current_db_version) :
    DatabaseTable(current_db_version),
    links_(std::make_unique<DatabaseServerPublisherLinks>(current_db_version)),
    amounts_(
        std::make_unique<DatabaseServerPublisherAmounts>(current_db_version)) {
}

DatabaseServerPublisherBanner::~DatabaseServerPublisherBanner() {
}

bool DatabaseServerPublisherBanner::Init(sql::Database* db) {
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

  success = links_->Init(db);
  if (!success) {
    return false;
  }

  success = amounts_->Init(db);
  if (!success) {
    return false;
  }

  return transaction.Commit();
}

bool DatabaseServerPublisherBanner::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  return CreateTableV15(db);
}

bool DatabaseServerPublisherBanner::CreateTableV7(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "publisher_key LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,"
      "title TEXT,"
      "description TEXT,"
      "background TEXT,"
      "logo TEXT,"
      "CONSTRAINT fk_%s_publisher_key"
      "    FOREIGN KEY (publisher_key)"
      "    REFERENCES server_publisher_info (publisher_key)"
      "    ON DELETE CASCADE"
      ")",
      table_name_,
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseServerPublisherBanner::CreateTableV15(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "publisher_key LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,"
      "title TEXT,"
      "description TEXT,"
      "background TEXT,"
      "logo TEXT"
      ")",
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseServerPublisherBanner::CreateIndex(sql::Database* db) {
  return CreateIndexV15(db);
}

bool DatabaseServerPublisherBanner::CreateIndexV7(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_key");
}

bool DatabaseServerPublisherBanner::CreateIndexV15(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_key");
}

bool DatabaseServerPublisherBanner::Migrate(
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


bool DatabaseServerPublisherBanner::MigrateToV7(sql::Database* db) {
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

bool DatabaseServerPublisherBanner::MigrateToV15(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  const std::string sql =
      "DROP INDEX IF EXISTS server_publisher_banner_publisher_key_index;";
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
    { "title", "title" },
    { "description", "description" },
    { "background", "background" },
    { "logo", "logo" }
  };

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }

  if (!links_->Migrate(db, 15)) {
    return false;
  }

  if (!amounts_->Migrate(db, 15)) {
    return false;
  }

  return true;
}

bool DatabaseServerPublisherBanner::InsertOrUpdate(
    sql::Database* db,
    ledger::ServerPublisherInfoPtr info) {
  if (!info || !info->banner) {
    return false;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_key, title, description, background, logo) "
      "VALUES (?, ?, ?, ?, ?)",
      table_name_);

  sql::Statement statment(
    db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statment.BindString(0, info->publisher_key);
  statment.BindString(1, info->banner->title);
  statment.BindString(2, info->banner->description);
  statment.BindString(3, info->banner->background);
  statment.BindString(4, info->banner->logo);

  if (!statment.Run()) {
    return false;
  }

  if (!links_->InsertOrUpdate(db, info->Clone())) {
    transaction.Rollback();
    return false;
  }

  if (!amounts_->InsertOrUpdate(db, info->Clone())) {
    transaction.Rollback();
    return false;
  }

  return transaction.Commit();
}

ledger::PublisherBannerPtr DatabaseServerPublisherBanner::GetRecord(
    sql::Database* db,
    const std::string& publisher_key) {
  const std::string query = base::StringPrintf(
      "SELECT title, description, background, logo "
      "FROM %s "
      "WHERE publisher_key=?",
      table_name_);

  sql::Statement statment(db->GetUniqueStatement(query.c_str()));
  statment.BindString(0, publisher_key);

  if (!statment.Step()) {
    return nullptr;
  }

  auto banner = ledger::PublisherBanner::New();
  banner->publisher_key = publisher_key;
  banner->title = statment.ColumnString(0);
  banner->description = statment.ColumnString(1);
  banner->background = statment.ColumnString(2);
  banner->logo = statment.ColumnString(3);
  banner->links = links_->GetRecord(db, publisher_key);
  banner->amounts = amounts_->GetRecord(db, publisher_key);

  return banner;
}

}  // namespace brave_rewards
