/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/database/database_contribution_queue.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards {

DatabaseContributionQueue::DatabaseContributionQueue(int current_db_version) :
    DatabaseTable(current_db_version),
    publishers_(std::make_unique<DatabaseContributionQueuePublishers>
        (current_db_version)) {
}

DatabaseContributionQueue::~DatabaseContributionQueue() {
}

std::string DatabaseContributionQueue::GetIdColumnName() {
  return base::StringPrintf("%s_id", table_name_);
}

bool DatabaseContributionQueue::Init(sql::Database* db) {
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

  success = publishers_->Init(db);
  if (!success) {
    return false;
  }

  return transaction.Commit();
}

bool DatabaseContributionQueue::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "%s INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
      "type INTEGER NOT NULL,"
      "amount DOUBLE NOT NULL,"
      "partial INTEGER NOT NULL DEFAULT 0,"
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL"
      ")",
      table_name_,
      GetIdColumnName().c_str());

  return db->Execute(query.c_str());
}

bool DatabaseContributionQueue::CreateIndex(sql::Database* db) {
  return true;
}

bool DatabaseContributionQueue::InsertOrUpdate(
    sql::Database* db,
    ledger::ContributionQueuePtr info) {
  if (!info) {
    return false;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }
  const std::string query = base::StringPrintf(
    "INSERT OR REPLACE INTO %s (%s, type, amount, partial) "
    "VALUES (?, ?, ?, ?)",
    table_name_,
    GetIdColumnName().c_str());

  sql::Statement statement(
      db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  if (info->id != 0) {
    statement.BindInt64(0, info->id);
  } else {
    statement.BindNull(0);
  }

  statement.BindInt(1, static_cast<int>(info->type));
  statement.BindDouble(2, info->amount);
  statement.BindBool(3, info->partial);

  if (!statement.Run()) {
    return false;
  }

  if (info->id == 0) {
    info->id = db->GetLastInsertRowId();
  }

  if (!publishers_->InsertOrUpdate(db, info->Clone())) {
    transaction.Rollback();
    return false;
  }

  return transaction.Commit();
}

ledger::ContributionQueuePtr DatabaseContributionQueue::GetFirstRecord(
    sql::Database* db) {
  const std::string query = base::StringPrintf(
      "SELECT %s, type, amount, partial FROM %s ORDER BY %s ASC LIMIT 1",
      GetIdColumnName().c_str(),
      table_name_,
      GetIdColumnName().c_str());

  sql::Statement statment(db->GetUniqueStatement(query.c_str()));

  if (!statment.Step()) {
    return nullptr;
  }

  auto info = ledger::ContributionQueue::New();
  info->id = statment.ColumnInt64(0);
  info->type = static_cast<ledger::RewardsType>(statment.ColumnInt(1));
  info->amount = statment.ColumnDouble(2);
  info->partial = static_cast<bool>(statment.ColumnInt(3));
  info->publishers = publishers_->GetRecords(db, info->id);

  return info;
}

bool DatabaseContributionQueue::DeleteRecord(
    sql::Database* db,
    const uint64_t id) {
  if (!db->Execute("PRAGMA foreign_keys=1;")) {
    LOG(ERROR) << "Error: deleting record for contribution queue with id" << id;
    return false;
  }

  const std::string query = base::StringPrintf(
      "DELETE FROM %s WHERE %s = ?",
      table_name_,
      GetIdColumnName().c_str());

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));
  statement.BindInt64(0, id);

  bool success = statement.Run();

  if (!db->Execute("PRAGMA foreign_keys=0;")) {
    return false;
  }

  return success;
}

}  // namespace brave_rewards
