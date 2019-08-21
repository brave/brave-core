/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/database/database_server_publisher_banner.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards {

DatabaseServerPublisherBanner::DatabaseServerPublisherBanner() {
}

DatabaseServerPublisherBanner::~DatabaseServerPublisherBanner() {
}

bool DatabaseServerPublisherBanner::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

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
  // TODO add amounts and social links

  return db->Execute(query.c_str());
}

bool DatabaseServerPublisherBanner::CreateIndex(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_key");
}

bool DatabaseServerPublisherBanner::InsertOrUpdate(
    sql::Database* db,
    ledger::ServerPublisherInfoPtr info) {
  if (!info || !info->banner) {
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

  return statment.Run();
}

}  // namespace brave_rewards
