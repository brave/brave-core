/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/browser/database/database_contribution_info.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {
  double ProbiToDouble(const std::string& probi) {
    const int32_t probi_size = 18;
    const size_t size = probi.size();
    std::string amount = "0";
    if (size > probi_size) {
      amount = probi;
      amount.insert(size - probi_size, ".");
    }

    return std::stod(amount);
  }
}  // namespace

namespace brave_rewards {

DatabaseContributionInfo::DatabaseContributionInfo(int current_db_version) :
    DatabaseTable(current_db_version),
    publishers_(std::make_unique<DatabaseContributionInfoPublishers>
        (current_db_version)) {
}

DatabaseContributionInfo::~DatabaseContributionInfo() = default;

bool DatabaseContributionInfo::Init(sql::Database* db) {
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

bool DatabaseContributionInfo::CreateTable(sql::Database* db) {
  return CreateTableV11(db);
}

bool DatabaseContributionInfo::CreateIndex(sql::Database* db) {
  return true;
}

bool DatabaseContributionInfo::CreateTableV2(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR,"
        "probi TEXT \"0\"  NOT NULL,"
        "date INTEGER NOT NULL,"
        "category INTEGER NOT NULL,"
        "month INTEGER NOT NULL,"
        "year INTEGER NOT NULL,"
        "CONSTRAINT fk_contribution_info_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseContributionInfo::CreateTableV8(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_id LONGVARCHAR,"
        "probi TEXT \"0\"  NOT NULL,"
        "date INTEGER NOT NULL,"
        "type INTEGER NOT NULL,"
        "month INTEGER NOT NULL,"
        "year INTEGER NOT NULL,"
        "CONSTRAINT fk_contribution_info_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE"
      ")",
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseContributionInfo::CreateTableV11(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    return true;
  }

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "contribution_id TEXT NOT NULL,"
        "amount DOUBLE NOT NULL,"
        "type INTEGER NOT NULL,"
        "step INTEGER NOT NULL DEFAULT -1,"
        "retry_count INTEGER NOT NULL DEFAULT -1,"
        "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (contribution_id)"
      ")",
      table_name_);

  return db->Execute(query.c_str());
}

bool DatabaseContributionInfo::CreateIndexV2(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_id");
}

bool DatabaseContributionInfo::CreateIndexV8(sql::Database* db) {
  return this->InsertIndex(db, table_name_, "publisher_id");
}

bool DatabaseContributionInfo::Migrate(sql::Database* db, const int target) {
  switch (target) {
    case 2: {
      return MigrateToV2(db);
    }
    case 8: {
      return MigrateToV8(db);
    }
    case 11: {
      return MigrateToV11(db);
    }
    default: {
      NOTREACHED();
      return false;
    }
  }
}

bool DatabaseContributionInfo::MigrateToV2(sql::Database* db) {
  if (db->DoesTableExist(table_name_)) {
    DropTable(db, table_name_);
  }

  if (!CreateTableV2(db)) {
    return false;
  }

  if (!CreateIndexV2(db)) {
    return false;
  }

  return true;
}

bool DatabaseContributionInfo::MigrateToV8(sql::Database* db) {
  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  const std::string sql =
      "DROP INDEX IF EXISTS contribution_info_publisher_id_index;";
  if (!db->Execute(sql.c_str())) {
    return false;
  }

  if (!CreateTableV8(db)) {
    return false;
  }

  if (!CreateIndexV8(db)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "publisher_id", "publisher_id" },
    { "probi", "probi" },
    { "date", "date" },
    { "category", "type" },
    { "month", "month" },
    { "year", "year" }
  };

  if (!MigrateDBTable(db, temp_table_name, table_name_, columns, true)) {
    return false;
  }

  return true;
}

bool DatabaseContributionInfo::MigrateToV11(sql::Database* db) {
  const std::string sql =
      "DROP INDEX IF EXISTS contribution_info_publisher_id_index;";
  if (!db->Execute(sql.c_str())) {
    return false;
  }

  const std::string temp_table_name = base::StringPrintf(
    "%s_temp",
    table_name_);

  if (!RenameDBTable(db, table_name_, temp_table_name)) {
    return false;
  }

  if (!CreateTableV11(db)) {
    return false;
  }

  if (!publishers_->Migrate(db, 11)) {
    return false;
  }

  const std::string query = base::StringPrintf(
    "SELECT publisher_id, probi, date, type FROM %s",
    temp_table_name.c_str());

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

  uint32_t count = 0;
  while (statement.Step()) {
    const std::string publisher_key = statement.ColumnString(0);
    const double amount = ProbiToDouble(statement.ColumnString(1));
    const auto date = statement.ColumnInt64(2);
    const int type = statement.ColumnInt(3);
    const std::string contribution_id = base::StringPrintf(
        "id_%s_%s",
        std::to_string(date).c_str(),
        std::to_string(count).c_str());

    std::string query = base::StringPrintf(
      "INSERT INTO %s "
      "(contribution_id, amount, type, step, "
      "retry_count, created_at) "
      "VALUES (?, ?, ?, ?, ?, ?)",
      table_name_);

    sql::Statement contribution(
        db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

    contribution.BindString(0, contribution_id);
    contribution.BindDouble(1, amount);
    contribution.BindInt(2, static_cast<int>(type));
    contribution.BindInt(3, -1);
    contribution.BindInt(4, -1);
    contribution.BindInt64(5, date);

    if (!contribution.Run()) {
      return false;
    }
    count++;

    if (publisher_key.empty()) {
      continue;
    }

    query =
      "INSERT INTO contribution_info_publishers "
      "(contribution_id, publisher_key, total_amount, contributed_amount) "
      "VALUES (?, ?, ?, ?)";

    sql::Statement publisher(
        db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

    publisher.BindString(0, contribution_id);
    publisher.BindString(1, publisher_key);
    publisher.BindDouble(2, amount);
    publisher.BindDouble(3, amount);

    if (!publisher.Run()) {
      return false;
    }
  }

  if (!DropTable(db, temp_table_name)) {
    return false;
  }

  return true;
}

bool DatabaseContributionInfo::InsertOrUpdate(
    sql::Database* db,
    ledger::ContributionInfoPtr info) {
  if (!info) {
    return false;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return false;
  }

  const std::string query = base::StringPrintf(
    "INSERT OR REPLACE INTO %s "
    "(contribution_id, amount, type, step, "
    "retry_count, created_at) "
    "VALUES (?, ?, ?, ?, ?, ?)",
    table_name_);

  sql::Statement statement(
      db->GetCachedStatement(SQL_FROM_HERE, query.c_str()));

  statement.BindString(0, info->contribution_id);
  statement.BindDouble(1, info->amount);
  statement.BindInt(2, static_cast<int>(info->type));
  statement.BindInt(3, info->step);
  statement.BindInt(4, info->retry_count);

  if (info->created_at == 0) {
    statement.BindNull(5);
  } else {
    statement.BindInt64(5, info->created_at);
  }

  if (!statement.Run()) {
    return false;
  }

  if (!publishers_->InsertOrUpdate(db, info->Clone())) {
    return false;
  }

  return transaction.Commit();
}

bool DatabaseContributionInfo::GetOneTimeTips(
    sql::Database* db,
    ledger::PublisherInfoList* list,
    const ledger::ActivityMonth month,
    const int year) {
  if (!list) {
    return false;
  }

  const std::string query =
    "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
    "ci.amount, ci.created_at, spi.status, pi.provider "
    "FROM contribution_info as ci "
    "INNER JOIN contribution_info_publishers AS cp "
    "ON cp.contribution_id = ci.contribution_id "
    "INNER JOIN publisher_info AS pi ON cp.publisher_key = pi.publisher_id "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = pi.publisher_id "
    "WHERE strftime('%m',  datetime(ci.created_at, 'unixepoch')) = ? AND "
    "strftime('%Y', datetime(ci.created_at, 'unixepoch')) = ? AND ci.type = ?";

  sql::Statement statement(db->GetUniqueStatement(query.c_str()));

  const std::string formatted_month = base::StringPrintf("%02d", month);

  statement.BindString(0, formatted_month);
  statement.BindString(1, std::to_string(year));
  statement.BindInt(2, static_cast<int>(ledger::RewardsType::ONE_TIME_TIP));

  while (statement.Step()) {
    auto publisher = ledger::PublisherInfo::New();

    publisher->id = statement.ColumnString(0);
    publisher->name = statement.ColumnString(1);
    publisher->url = statement.ColumnString(2);
    publisher->favicon_url = statement.ColumnString(3);
    publisher->weight = statement.ColumnDouble(4);
    publisher->reconcile_stamp = statement.ColumnInt64(5);
    publisher->status =
        static_cast<ledger::mojom::PublisherStatus>(statement.ColumnInt64(6));
    publisher->provider = statement.ColumnString(7);

    list->push_back(std::move(publisher));
  }

  return true;
}

}  // namespace brave_rewards
