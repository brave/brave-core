/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/publisher_info_database.h"

#include <stdint.h>

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "build/build_config.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"


namespace brave_rewards {

namespace {

const int kCurrentVersionNumber = 1;
const int kCompatibleVersionNumber = 1;

}  // namespace

PublisherInfoDatabase::PublisherInfoDatabase(const base::FilePath& db_path) :
    db_path_(db_path),
    initialized_(false) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

PublisherInfoDatabase::~PublisherInfoDatabase() {
}

bool PublisherInfoDatabase::Init() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (initialized_)
    return true;

  if (!db_.Open(db_path_))
    return false;

  sql::Transaction committer(&db_);
  if (!committer.Begin())
    return false;

  if (!meta_table_.Init(&db_, GetCurrentVersion(), kCompatibleVersionNumber))
    return false;
  if (!CreatePublisherInfoTable() || !CreateContributionInfoTable())
    return false;

  CreateContributionInfoIndex();

  // Version check.
  sql::InitStatus version_status = EnsureCurrentVersion();
  if (version_status != sql::INIT_OK)
    return version_status;

  if (!committer.Commit())
    return false;

  memory_pressure_listener_.reset(new base::MemoryPressureListener(
      base::Bind(&PublisherInfoDatabase::OnMemoryPressure,
      base::Unretained(this))));

  initialized_ = true;
  return initialized_;
}

bool PublisherInfoDatabase::CreateContributionInfoTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "contribution_info";
  if (GetDB().DoesTableExist(name))
    return true;

  // Note: revise implementation for InsertOrUpdateRowByID() if you add any
  // new constraints to the schema.
  std::string sql;
  sql.append("CREATE TABLE ");
  sql.append(name);
  sql.append(
      "("
      "publisher_id LONGVARCHAR NOT NULL,"
      "value DOUBLE DEFAULT 0 NOT NULL,"
      "date INTEGER DEFAULT 0 NOT NULL)");
  return GetDB().Execute(sql.c_str());
}

bool PublisherInfoDatabase::CreateContributionInfoIndex() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return GetDB().Execute(
      "CREATE INDEX IF NOT EXISTS contribution_info_publisher_id_index "
      "ON contribution_info (publisher_id)");
}

bool PublisherInfoDatabase::CreatePublisherInfoTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "publisher_info";
  if (GetDB().DoesTableExist(name))
    return true;

  // Update InsertOrUpdatePublisherInfo() if you add anything here
  std::string sql;
  sql.append("CREATE TABLE ");
  sql.append(name);
  sql.append(
      "("
      "id LONGVARCHAR PRIMARY KEY,"
      "duration INTEGER DEFAULT 0 NOT NULL,"
      "score DOUBLE DEFAULT 0 NOT NULL,"
      "pinned BOOLEAN DEFAULT 0 NOT NULL,"
      "percent INTEGER DEFAULT 0 NOT NULL,"
      "weight DOUBLE DEFAULT 0 NOT NULL,"
      "excluded BOOLEAN DEFAULT 0 NOT NULL,"
      "category INTEGER NOT NULL,"
      "month INTEGER NOT NULL,"
      "year INTEGER NOT NULL)");
  return GetDB().Execute(sql.c_str());
}

bool PublisherInfoDatabase::InsertOrUpdatePublisherInfo(
    const ledger::PublisherInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "INSERT OR REPLACE INTO publisher_info "
      "(id, duration, score, pinned, percent, "
      "weight, excluded, category, month, year) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));

  statement.BindString(0, info.id);
  statement.BindInt64(1, (int)info.duration);
  statement.BindDouble(2, info.score);
  statement.BindBool(3, info.pinned);
  statement.BindInt64(4, (int)info.percent);
  statement.BindDouble(5, info.weight);
  statement.BindBool(6, info.excluded);
  statement.BindInt(7, info.category);
  statement.BindInt(8, info.month);
  statement.BindInt(9, info.year);

  return statement.Run();
}

bool PublisherInfoDatabase::Find(int start,
                                 int limit,
                                 const ledger::PublisherInfoFilter& filter,
                                 ledger::PublisherInfoList* list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  CHECK(list);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  std::string query = "SELECT id, duration, score, pinned, percent, "
      "weight, excluded, category, month, year "
      "FROM publisher_info "
      "WHERE 1 = 1";

  if (!filter.id.empty())
    query += " AND id = ? ";

  if (filter.category != ledger::PUBLISHER_CATEGORY::ALL_CATEGORIES)
    query += " AND category = ? ";

  if (filter.month != ledger::PUBLISHER_MONTH::ANY)
    query += " AND month = ? ";

  if (filter.year > 0)
    query += " AND year = ? ";

  if (start > 1)
    query += " OFFSET " + std::to_string(start);

  if (limit > 0)
    query += " LIMIT " + std::to_string(limit);

  for (const auto& it : filter.order_by) {
    query += " ORDER BY " + it.first;
    query += (it.second ? "ASC" : "DESC");
  }

  sql::Statement info_sql(db_.GetUniqueStatement(query.c_str()));

  int column = 0;
  if (!filter.id.empty())
    info_sql.BindString(column++, filter.id);

  if (filter.category != ledger::PUBLISHER_CATEGORY::ALL_CATEGORIES)
    info_sql.BindInt(column++, filter.category);

  if (filter.month != ledger::PUBLISHER_MONTH::ANY)
    info_sql.BindInt(column++, filter.month);

  if (filter.year > 0)
    info_sql.BindInt(column++, filter.year);

  while (info_sql.Step()) {
    std::string id(info_sql.ColumnString(0));
    ledger::PUBLISHER_MONTH month(
        static_cast<ledger::PUBLISHER_MONTH>(info_sql.ColumnInt(8)));
    int year(info_sql.ColumnInt(9));

    ledger::PublisherInfo info(id, month, year);
    info.duration = info_sql.ColumnInt64(1);

    info.score = info_sql.ColumnDouble(2);
    info.pinned = info_sql.ColumnBool(3);
    info.percent = info_sql.ColumnInt64(4);
    info.weight = info_sql.ColumnDouble(5);
    info.excluded = info_sql.ColumnBool(6);
    info.category =
        static_cast<ledger::PUBLISHER_CATEGORY>(info_sql.ColumnInt(7));

    list->push_back(info);
  }

  return list;
}

// static
int PublisherInfoDatabase::GetCurrentVersion() {
  return kCurrentVersionNumber;
}

void PublisherInfoDatabase::Vacuum() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!initialized_)
    return;

  DCHECK_EQ(0, db_.transaction_nesting()) <<
      "Can not have a transaction when vacuuming.";
  ignore_result(db_.Execute("VACUUM"));
}

void PublisherInfoDatabase::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool trim_aggressively =
      memory_pressure_level ==
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL;
  db_.TrimMemory(trim_aggressively);
}

std::string PublisherInfoDatabase::GetDiagnosticInfo(int extended_error,
                                               sql::Statement* statement) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(initialized_);
  return db_.GetDiagnosticInfo(extended_error, statement);
}

sql::Database& PublisherInfoDatabase::GetDB() {
  return db_;
}

sql::MetaTable& PublisherInfoDatabase::GetMetaTable() {
  return meta_table_;
}

// Migration -------------------------------------------------------------------

sql::InitStatus PublisherInfoDatabase::EnsureCurrentVersion() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // We can't read databases newer than we were designed for.
  if (meta_table_.GetCompatibleVersionNumber() > kCurrentVersionNumber) {
    LOG(WARNING) << "Publisher info database is too new.";
    return sql::INIT_TOO_NEW;
  }

  int cur_version = meta_table_.GetVersionNumber();

  // Put migration code here

  // When the version is too old, we just try to continue anyway, there should
  // not be a released product that makes a database too old for us to handle.
  LOG_IF(WARNING, cur_version < GetCurrentVersion()) <<
         "History database version " << cur_version << " is too old to handle.";

  return sql::INIT_OK;
}

}  // namespace history
