/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/database/database_server_publisher_links.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards {

DatabaseServerPublisherLinks::DatabaseServerPublisherLinks(
    int current_db_version) :
    DatabaseTable(current_db_version) {
}

DatabaseServerPublisherLinks::~DatabaseServerPublisherLinks() {
}

bool DatabaseServerPublisherLinks::Init(sql::Database* db) {
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

bool DatabaseServerPublisherLinks::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

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

bool DatabaseServerPublisherLinks::CreateIndex(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_key");
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
