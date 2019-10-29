/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/database/database_unblinded_token.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards {

DatabaseUnblindedToken::DatabaseUnblindedToken(
    int current_db_version) :
    DatabaseTable(current_db_version) {
}

DatabaseUnblindedToken::~DatabaseUnblindedToken() {
}

bool DatabaseUnblindedToken::Init(sql::Database* db) {
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

bool DatabaseUnblindedToken::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "token_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
        "token_value TEXT,"
        "public_key TEXT,"
        "value DOUBLE NOT NULL DEFAULT 0,"
        "promotion_id TEXT,"
        "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "CONSTRAINT fk_%s_promotion_id "
          "FOREIGN KEY (promotion_id) "
          "REFERENCES promotion (promotion_id) ON DELETE CASCADE"
      ")",
      table_name_,
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseUnblindedToken::CreateIndex(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "token_id");
}

bool DatabaseUnblindedToken::InsertOrUpdate(
    sql::Database* db,
    ledger::UnblindedTokenPtr info) {
  if (!info) {
    return false;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(token_id, token_value, public_key, value, promotion_id) "
      "VALUES (?, ?, ?, ?, ?)",
      table_name_);

  sql::Statement statement(
    db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statement.BindInt64(0, info->id);
  statement.BindString(1, info->token_value);
  statement.BindString(2, info->public_key);
  statement.BindDouble(3, info->value);
  statement.BindString(4, info->promotion_id);

  if (!statement.Run()) {
    return false;
  }

  return transaction.Commit();
}

ledger::UnblindedTokenList DatabaseUnblindedToken::GetAllRecords(
    sql::Database* db) {
  ledger::UnblindedTokenList list;
  const std::string query = base::StringPrintf(
      "SELECT token_id, token_value, public_key, value, promotion_id FROM %s",
      table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

  while (statement.Step()) {
    auto info = ledger::UnblindedToken::New();
    info->id = statement.ColumnInt64(0);
    info->token_value = statement.ColumnString(1);
    info->public_key = statement.ColumnString(2);
    info->value = statement.ColumnDouble(3);
    info->promotion_id = statement.ColumnString(4);

    list.push_back(std::move(info));
  }

  return list;
}

}  // namespace brave_rewards
