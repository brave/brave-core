/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/database/database_server_publisher_info.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards {

DatabaseServerPublisherInfo::DatabaseServerPublisherInfo() :
    banner_(std::make_unique<DatabaseServerPublisherBanner>()),
    links_(std::make_unique<DatabaseServerPublisherLinks>()),
    amounts_(std::make_unique<DatabaseServerPublisherAmounts>()) {
}

DatabaseServerPublisherInfo::~DatabaseServerPublisherInfo() {
}

bool DatabaseServerPublisherInfo::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
      "("
      "publisher_key LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,"
      "verified BOOLEAN DEFAULT 0 NOT NULL,"
      "excluded INTEGER DEFAULT 0 NOT NULL,"
      "addresses TEXT NOT NULL"
      ")",
      table_name_);

  bool success = db->Execute(query.c_str());
  if (!success) {
    return false;
  }

  success = banner_->CreateTable(db);
  if (!success) {
    return false;
  }

  success = links_->CreateTable(db);
  if (!success) {
    return false;
  }

  success = amounts_->CreateTable(db);

  return success;
}

bool DatabaseServerPublisherInfo::CreateIndex(sql::Database* db) {
  bool success = this->InsertIndex(db, table_name_, "publisher_key");
  if (!success) {
    return false;
  }

  success = banner_->CreateIndex(db);
  if (!success) {
    return false;
  }

  success = links_->CreateIndex(db);
  if (!success) {
    return false;
  }

  success = amounts_->CreateIndex(db);

  return success;
}

bool DatabaseServerPublisherInfo::InsertOrUpdate(
    sql::Database* db,
    ledger::ServerPublisherInfoPtr info) {
  if (!info) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_key, verified, excluded, addresses) "
      "VALUES (?, ?, ?, ?)",
      table_name_);

  sql::Statement statment(
    db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statment.BindString(0, info->publisher_key);
  statment.BindBool(1, info->verified);
  statment.BindBool(2, info->excluded);
  statment.BindString(3, info->address);

  return statment.Run();
}

bool DatabaseServerPublisherInfo::ClearAndInsertList(
    sql::Database* db,
    const ledger::ServerPublisherInfoList& list) {
  const std::string clear_query = base::StringPrintf(
      "DELETE FROM %s",
      table_name_);

  bool success = db->Execute(clear_query.c_str());
  if (!success) {
    return false;
  }

  if (list.size() == 0) {
    return true;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  for (const auto& info : list) {
    if (!info) {
      continue;
    }

    if (!InsertOrUpdate(db, info->Clone())) {
      transaction.Rollback();
      return false;
    }

    if (!banner_->InsertOrUpdate(db, info->Clone())) {
      transaction.Rollback();
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
  }

  return transaction.Commit();
}

}  // namespace brave_rewards
