/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/database/database_promotion.h"

#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {
  const char* table_name_ = "promotion";
  const int minimum_version_ = 10;
}  // namespace

namespace brave_rewards {

DatabasePromotion::DatabasePromotion(int current_db_version) :
    DatabaseTable(current_db_version),
    creds_(std::make_unique<DatabasePromotionCreds>(current_db_version)) {
}

DatabasePromotion::~DatabasePromotion() {
}

bool DatabasePromotion::Init(sql::Database* db) {
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

  success = creds_->Init(db);
  if (!success) {
    return false;
  }

  return transaction.Commit();
}

bool DatabasePromotion::CreateTable(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  const int version = GetCurrentDBVersion();

  if (version >= 13) {
    return CreateTableV13(db);
  }

  if (version >= 10) {
    return CreateTableV10(db);
  }

  NOTREACHED();
  return false;
}

bool DatabasePromotion::CreateTableV10(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "%s_id TEXT NOT NULL,"
        "version INTEGER NOT NULL,"
        "type INTEGER NOT NULL,"
        "public_keys TEXT NOT NULL,"
        "suggestions INTEGER NOT NULL DEFAULT 0,"
        "approximate_value DOUBLE NOT NULL DEFAULT 0,"
        "status INTEGER NOT NULL DEFAULT 0,"
        "expires_at TIMESTAMP NOT NULL,"
        "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (%s_id)"
      ")",
      table_name_,
      table_name_,
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabasePromotion::CreateTableV13(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "%s_id TEXT NOT NULL,"
        "version INTEGER NOT NULL,"
        "type INTEGER NOT NULL,"
        "public_keys TEXT NOT NULL,"
        "suggestions INTEGER NOT NULL DEFAULT 0,"
        "approximate_value DOUBLE NOT NULL DEFAULT 0,"
        "status INTEGER NOT NULL DEFAULT 0,"
        "expires_at TIMESTAMP NOT NULL,"
        "claimed_at TIMESTAMP,"
        "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (%s_id)"
      ")",
      table_name_,
      table_name_,
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabasePromotion::CreateIndex(sql::Database* db) {
  return CreateIndexV13(db);
}

bool DatabasePromotion::CreateIndexV10(sql::Database* db) {
  const std::string id = base::StringPrintf("%s_id", table_name_);
  return this->InsertIndex(db, table_name_, id);
}

bool DatabasePromotion::CreateIndexV13(sql::Database* db) {
  const std::string id = base::StringPrintf("%s_id", table_name_);
  return this->InsertIndex(db, table_name_, id);
}

bool DatabasePromotion::Migrate(sql::Database* db, const int target) {
  switch (target) {
    case 10: {
      return MigrateToV10(db);
    }
    case 13: {
      return MigrateToV13(db);
    }
    default: {
      NOTREACHED();
      return false;
    }
  }
}

bool DatabasePromotion::MigrateToV10(sql::Database* db) {
  if (!CreateTableV10(db)) {
    return false;
  }

  if (!CreateIndexV10(db)) {
    return false;
  }

  return true;
}

bool DatabasePromotion::MigrateToV13(sql::Database* db) {
  if (!db->DoesTableExist(table_name_)) {
    if (!CreateTableV13(db)) {
      return false;
    }
  }

  const char column[] = "claimed_at";
  if (!db->DoesColumnExist(table_name_, column)) {
    const std::string query = base::StringPrintf(
        "ALTER TABLE %s ADD %s TIMESTAMP",
        table_name_,
        column);

    sql::Statement statement(
        db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

    return statement.Run();
  }

  return true;
}

bool DatabasePromotion::InsertOrUpdate(
    sql::Database* db,
    ledger::PromotionPtr info) {
  if (!info) {
    return false;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(%s_id, version, type, public_keys, suggestions, "
      "approximate_value, status, expires_at, claimed_at) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
      table_name_,
      table_name_);

  sql::Statement statement(
    db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statement.BindString(0, info->id);
  statement.BindInt(1, info->version);
  statement.BindInt(2, static_cast<int>(info->type));
  statement.BindString(3, info->public_keys);
  statement.BindInt64(4, info->suggestions);
  statement.BindDouble(5, info->approximate_value);
  statement.BindInt(6, static_cast<int>(info->status));
  statement.BindInt64(7, info->expires_at);
  statement.BindInt64(8, info->claimed_at);

  if (!statement.Run()) {
    return false;
  }

  if (info->credentials) {
    if (!creds_->InsertOrUpdate(db, info->credentials->Clone(), info->id)) {
      return false;
    }
  }

  return transaction.Commit();
}

ledger::PromotionPtr DatabasePromotion::GetRecord(
    sql::Database* db,
    const std::string& id) {
  if (id.empty()) {
    return nullptr;
  }

  const std::string query = base::StringPrintf(
      "SELECT %s_id, version, type, public_keys, suggestions, "
      "approximate_value, status, expires_at, claimed_at FROM %s WHERE %s_id=?",
      table_name_,
      table_name_,
      table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));
  statement.BindString(0, id);

  if (!statement.Step()) {
    return nullptr;
  }

  auto info = ledger::Promotion::New();
  info->id = statement.ColumnString(0);
  info->version = statement.ColumnInt(1);
  info->type = static_cast<ledger::PromotionType>(statement.ColumnInt(2));
  info->public_keys = statement.ColumnString(3);
  info->suggestions = statement.ColumnInt64(4);
  info->approximate_value = statement.ColumnDouble(5);
  info->status = static_cast<ledger::PromotionStatus>(statement.ColumnInt(6));
  info->expires_at = statement.ColumnInt64(7);
  info->claimed_at = statement.ColumnInt64(8);
  info->credentials = creds_->GetRecord(db, info->id);

  return info;
}

ledger::PromotionMap DatabasePromotion::GetAllRecords(sql::Database* db) {
  const std::string query = base::StringPrintf(
      "SELECT %s_id, version, type, public_keys, suggestions, "
      "approximate_value, status, expires_at, claimed_at FROM %s",
      table_name_,
      table_name_);

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

  ledger::PromotionMap map;

  while (statement.Step()) {
    auto info = ledger::Promotion::New();
    info->id = statement.ColumnString(0);
    info->version = statement.ColumnInt(1);
    info->type = static_cast<ledger::PromotionType>(statement.ColumnInt(2));
    info->public_keys = statement.ColumnString(3);
    info->suggestions = statement.ColumnInt64(4);
    info->approximate_value = statement.ColumnDouble(5);
    info->status = static_cast<ledger::PromotionStatus>(statement.ColumnInt(6));
    info->expires_at = statement.ColumnInt64(7);
    info->claimed_at = statement.ColumnInt64(8);
    info->credentials = creds_->GetRecord(db, info->id);

    map.insert(std::make_pair(info->id, std::move(info)));
  }

  return map;
}

}  // namespace brave_rewards
