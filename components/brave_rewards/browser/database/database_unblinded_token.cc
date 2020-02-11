/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_rewards/browser/database/database_unblinded_token.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_rewards {

namespace  {

const char* table_name_ = "unblinded_tokens";

int64_t GetExpirationDate(const int32_t type, const int64_t stamp) {
  const auto promotion_type = static_cast<ledger::PromotionType>(type);
  if (promotion_type == ledger::PromotionType::ADS) {
    return 0;
  }

  return stamp;
}

}  // namespace

DatabaseUnblindedToken::DatabaseUnblindedToken(
    int current_db_version) :
    DatabaseTable(current_db_version) {
}

DatabaseUnblindedToken::~DatabaseUnblindedToken() {
}

bool DatabaseUnblindedToken::CreateTableV10(sql::Database* db) {
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

bool DatabaseUnblindedToken::CreateTableV15(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "token_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
        "token_value TEXT,"
        "public_key TEXT,"
        "value DOUBLE NOT NULL DEFAULT 0,"
        "promotion_id TEXT,"
        "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"
      ")",
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseUnblindedToken::CreateIndexV10(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "token_id");
}

bool DatabaseUnblindedToken::CreateIndexV15(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "promotion_id");
}

bool DatabaseUnblindedToken::Migrate(sql::Database* db, const int target) {
  switch (target) {
    case 10: {
      return MigrateToV10(db);
    }
    case 14: {
      return MigrateToV14(db);
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

bool DatabaseUnblindedToken::MigrateToV10(sql::Database* db) {
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

bool DatabaseUnblindedToken::MigrateToV14(sql::Database* db) {
  DCHECK(db);
  if (!db) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET value = 0.25",
      table_name_);

  sql::Statement statement(
    db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  return statement.Run();
}

bool DatabaseUnblindedToken::MigrateToV15(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  const std::string sql =
      "DROP INDEX IF EXISTS unblinded_tokens_token_id_index;";
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
    { "token_id", "token_id" },
    { "token_value", "token_value" },
    { "public_key", "public_key" },
    { "value", "value" },
    { "promotion_id", "promotion_id" },
    { "created_at", "created_at" }
  };

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }

  return true;
}

bool DatabaseUnblindedToken::InsertOrUpdateList(
    sql::Database* db,
    ledger::UnblindedTokenList list) {
  if (list.size() == 0) {
    return false;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  for (const auto& info : list) {
    const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(token_id, token_value, public_key, value, promotion_id) "
      "VALUES (?, ?, ?, ?, ?)",
      table_name_);

    sql::Statement statement(
        db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

    if (info->id != 0) {
      statement.BindInt64(0, info->id);
    } else {
      statement.BindNull(0);
    }

    statement.BindString(1, info->token_value);
    statement.BindString(2, info->public_key);
    statement.BindDouble(3, info->value);
    statement.BindString(4, info->promotion_id);

    if (!statement.Run()) {
      return false;
    }
  }

  return transaction.Commit();
}

ledger::UnblindedTokenList DatabaseUnblindedToken::GetAllRecords(
    sql::Database* db) {
  ledger::UnblindedTokenList list;
  const std::string query = base::StringPrintf(
      "SELECT u.token_id, u.token_value, u.public_key, u.value, "
      "u.promotion_id, p.expires_at, p.type FROM %s as u "
      "LEFT JOIN promotion as p ON p.promotion_id = u.promotion_id",
      table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

  while (statement.Step()) {
    auto info = ledger::UnblindedToken::New();
    info->id = statement.ColumnInt64(0);
    info->token_value = statement.ColumnString(1);
    info->public_key = statement.ColumnString(2);
    info->value = statement.ColumnDouble(3);
    info->promotion_id = statement.ColumnString(4);
    info->expires_at =
        GetExpirationDate(statement.ColumnInt(6), statement.ColumnInt64(5));

    list.push_back(std::move(info));
  }

  return list;
}

bool DatabaseUnblindedToken::DeleteRecords(
    sql::Database* db,
    const std::vector<std::string>& id_list) {
  if (id_list.size() == 0) {
    return true;
  }

  const std::string query = base::StringPrintf(
      "DELETE FROM %s WHERE token_id IN (%s)",
      table_name_,
      base::JoinString(id_list, ", ").c_str());

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

  return statement.Run();
}

// static
bool DatabaseUnblindedToken::DeleteRecordsForPromotion(
    sql::Database* db,
    const std::string& promotion_id) {
  if (promotion_id.empty()) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "DELETE FROM %s WHERE promotion_id = '%s'",
      table_name_,
      promotion_id.c_str());

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

  return statement.Run();
}

}  // namespace brave_rewards
