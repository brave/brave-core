/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/database/database_promotion_creds.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards {

DatabasePromotionCreds::DatabasePromotionCreds(
    int current_db_version) :
    DatabaseTable(current_db_version) {
}

DatabasePromotionCreds::~DatabasePromotionCreds() {
}

bool DatabasePromotionCreds::Init(sql::Database* db) {
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

bool DatabasePromotionCreds::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "%s_id TEXT NOT NULL,"
        "blinded_creds TEXT NOT NULL,"
        "signed_creds TEXT,"
        "public_key TEXT,"
        "batch_proof TEXT,"
        "claim_id TEXT,"
        "CONSTRAINT fk_%s_%s_id "
          "FOREIGN KEY (%s_id) "
          "REFERENCES %s (%s_id) ON DELETE CASCADE"
      ")",
      table_name_,
      parent_table_name_,
      table_name_,
      parent_table_name_,
      parent_table_name_,
      parent_table_name_,
      parent_table_name_);

  return db->Execute(query.c_str());
}

bool DatabasePromotionCreds::CreateIndex(sql::Database* db) {
  const std::string id = base::StringPrintf("%s_id", parent_table_name_);
  return this->InsertIndex(db, table_name_, id);
}

bool DatabasePromotionCreds::InsertOrUpdate(
    sql::Database* db,
    ledger::PromotionCredsPtr info,
    const std::string& promotion_id) {
  if (!info || promotion_id.empty()) {
    return false;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(%s_id, blinded_creds, signed_creds, public_key, batch_proof, claim_id) "
      "VALUES (?, ?, ?, ?, ?, ?)",
      table_name_,
      parent_table_name_);

  sql::Statement statement(
    db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statement.BindString(0, promotion_id);
  statement.BindString(1, info->blinded_creds);
  statement.BindString(2, info->signed_creds);
  statement.BindString(3, info->public_key);
  statement.BindString(4, info->batch_proof);
  statement.BindString(5, info->claim_id);

  if (!statement.Run()) {
    return false;
  }

  return transaction.Commit();
}

ledger::PromotionCredsPtr DatabasePromotionCreds::GetRecord(
    sql::Database* db,
    const std::string& promotion_id) {
  if (promotion_id.empty()) {
    return nullptr;
  }

  const std::string query = base::StringPrintf(
      "SELECT blinded_creds, signed_creds, public_key, "
      "batch_proof, claim_id FROM %s WHERE id=?",
      table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));
  statement.BindString(0, promotion_id);

  if (!statement.Step()) {
    return nullptr;
  }

  auto info = ledger::PromotionCreds::New();
  info->blinded_creds = statement.ColumnString(0);
  info->signed_creds = statement.ColumnString(1);
  info->public_key = statement.ColumnString(2);
  info->batch_proof = statement.ColumnString(3);
  info->claim_id = statement.ColumnString(4);

  return info;
}

}  // namespace brave_rewards
