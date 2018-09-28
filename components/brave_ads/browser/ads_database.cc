/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>
#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "build/build_config.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"

#include "ads_database.h"

namespace brave_ads {

namespace {

const int kCurrentVersionNumber = 1;
const int kCompatibleVersionNumber = 1;

}  // namespace

AdsDatabase::AdsDatabase(const base::FilePath& db_path) :
    db_path_(db_path),
    initialized_(false) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

AdsDatabase::~AdsDatabase() {
}

bool AdsDatabase::Init() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (initialized_) {
    return true;
  }

  if (!db_.Open(db_path_)) {
    LOG(ERROR) << "Could not open db";
    return false;
  }

  // TODO - add error delegate
  sql::Transaction committer(&db_);
  if (!committer.Begin())
    return false;

  if (!meta_table_.Init(&db_, GetCurrentVersion(), kCompatibleVersionNumber))
    return false;
  if (!CreateAdsHistoryTable())
    return false;

  CreateHistoryIndex();

  // Version check.
  sql::InitStatus version_status = EnsureCurrentVersion();
  if (version_status != sql::INIT_OK)
    return version_status;

  if (!committer.Commit())
    return false;

  memory_pressure_listener_.reset(new base::MemoryPressureListener(
      base::Bind(&AdsDatabase::OnMemoryPressure,
      base::Unretained(this))));

  initialized_ = true;
  return initialized_;
}

bool AdsDatabase::CreateAdsHistoryTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "ads_history";
  if (GetDB().DoesTableExist(name))
    return true;

  std::string sql;
  sql.append("CREATE TABLE ");
  sql.append(name);
  sql.append(
      "("
      "uuid LONGVARCHAR NOT NULL,"
      "date INTEGER DEFAULT 0 NOT NULL)");
  return GetDB().Execute(sql.c_str());
}

bool AdsDatabase::CreateHistoryIndex() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return GetDB().Execute(
      "CREATE INDEX IF NOT EXISTS ads_history_date_index "
      "ON ads_history (date)");
}

bool AdsDatabase::PushToHistory(
    const usermodel::Ad& ad) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  if (!initialized) {
    LOG(ERROR) << "Could not initialize";
  }
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "INSERT OR REPLACE INTO ads_history "
      "(uuid, date) "
      "VALUES (?, ?)"));

  statement.BindString(0, ad.uuid);
  statement.BindString(1, std::to_string(base::Time::NowFromSystemTime().ToTimeT()));

  return statement.Run();
}

bool AdsDatabase::AdsSeen(time_t timestamp, std::set<std::string> *set) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  CHECK(set);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  std::string query = "SELECT uuid FROM ads_history WHERE date >= " + std::to_string(timestamp);
  sql::Statement info_sql(db_.GetUniqueStatement(query.c_str()));

  while (info_sql.Step()) {
    std::string id(info_sql.ColumnString(0));
    set->insert(id);
  }

  return set;
}

// static
int AdsDatabase::GetCurrentVersion() {
  return kCurrentVersionNumber;
}

void AdsDatabase::Vacuum() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!initialized_)
    return;

  DCHECK_EQ(0, db_.transaction_nesting()) <<
      "Can not have a transaction when vacuuming.";
  ignore_result(db_.Execute("VACUUM"));
}

void AdsDatabase::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool trim_aggressively =
      memory_pressure_level ==
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL;
  db_.TrimMemory(trim_aggressively);
}

std::string AdsDatabase::GetDiagnosticInfo(int extended_error,
                                               sql::Statement* statement) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(initialized_);
  return db_.GetDiagnosticInfo(extended_error, statement);
}

sql::Database& AdsDatabase::GetDB() {
  return db_;
}

sql::MetaTable& AdsDatabase::GetMetaTable() {
  return meta_table_;
}

// Migration -------------------------------------------------------------------

sql::InitStatus AdsDatabase::EnsureCurrentVersion() {
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
