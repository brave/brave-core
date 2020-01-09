/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/browser/database/database_promotion_creds.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {
  const char table_name_[] = "promotion_creds";
  const int minimum_version_ = 10;
  const char parent_table_name_[] = "promotion";
}  // namespace

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

  return CreateTableV15(db);
}

bool DatabasePromotionCreds::CreateTableV10(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "%s_id TEXT UNIQUE NOT NULL,"
        "tokens TEXT NOT NULL,"
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

bool DatabasePromotionCreds::CreateTableV15(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "%s_id TEXT UNIQUE NOT NULL,"
        "tokens TEXT NOT NULL,"
        "blinded_creds TEXT NOT NULL,"
        "signed_creds TEXT,"
        "public_key TEXT,"
        "batch_proof TEXT,"
        "claim_id TEXT"
      ")",
      table_name_,
      parent_table_name_);

  return db->Execute(query.c_str());
}

bool DatabasePromotionCreds::CreateIndex(sql::Database* db) {
  return CreateIndexV15(db);
}

bool DatabasePromotionCreds::CreateIndexV10(sql::Database* db) {
  const std::string id = base::StringPrintf("%s_id", parent_table_name_);
  return this->InsertIndex(db, table_name_, id);
}

bool DatabasePromotionCreds::CreateIndexV15(sql::Database* db) {
  const std::string id = base::StringPrintf("%s_id", parent_table_name_);
  return this->InsertIndex(db, table_name_, id);
}

bool DatabasePromotionCreds::Migrate(sql::Database* db, const int target) {
  switch (target) {
    case 10: {
      return MigrateToV10(db);
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

bool DatabasePromotionCreds::MigrateToV10(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    DropTable(db, table_name_);
  }

  if (!CreateTableV10(db)) {
    return false;
  }

  if (!CreateIndexV10(db)) {
    return false;
  }

  return true;
}

bool DatabasePromotionCreds::MigrateToV15(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  const std::string sql =
      "DROP INDEX IF EXISTS promotion_creds_promotion_id_index;";
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
    { "promotion_id", "promotion_id" },
    { "tokens", "tokens" },
    { "blinded_creds", "blinded_creds" },
    { "signed_creds", "signed_creds" },
    { "public_key", "public_key" },
    { "batch_proof", "batch_proof" },
    { "claim_id", "claim_id" }
  };

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }
  return true;
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
      "(%s_id, tokens, blinded_creds, signed_creds, "
      "public_key, batch_proof, claim_id) "
      "VALUES (?, ?, ?, ?, ?, ?, ?)",
      table_name_,
      parent_table_name_);

  sql::Statement statement(
    db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statement.BindString(0, promotion_id);
  statement.BindString(1, info->tokens);
  statement.BindString(2, info->blinded_creds);
  statement.BindString(3, info->signed_creds);
  statement.BindString(4, info->public_key);
  statement.BindString(5, info->batch_proof);
  statement.BindString(6, info->claim_id);

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
      "SELECT tokens, blinded_creds, signed_creds, public_key, "
      "batch_proof, claim_id FROM %s WHERE %s_id=?",
      table_name_,
      parent_table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));
  statement.BindString(0, promotion_id);

  if (!statement.Step()) {
    return nullptr;
  }

  auto info = ledger::PromotionCreds::New();
  info->tokens = statement.ColumnString(0);
  info->blinded_creds = statement.ColumnString(1);
  info->signed_creds = statement.ColumnString(2);
  info->public_key = statement.ColumnString(3);
  info->batch_proof = statement.ColumnString(4);
  info->claim_id = statement.ColumnString(5);

  return info;
}

}  // namespace brave_rewards
