/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/bundle_state_database.h"

#include <stdint.h>

#include <string>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "build/build_config.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_ads {

namespace {

const int kCurrentVersionNumber = 1;
const int kCompatibleVersionNumber = 1;

}  // namespace

BundleStateDatabase::BundleStateDatabase(const base::FilePath& db_path) :
    db_path_(db_path),
    initialized_(false) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

BundleStateDatabase::~BundleStateDatabase() {
}

bool BundleStateDatabase::Init() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (initialized_)
    return true;

  if (!db_.Open(db_path_))
    return false;

  // TODO - add error delegate
  sql::Transaction committer(&db_);
  if (!committer.Begin())
    return false;

  if (!meta_table_.Init(&db_, GetCurrentVersion(), kCompatibleVersionNumber))
    return false;
  if (!CreateCategoryTable() ||
      !CreateAdInfoTable() ||
      !CreateAdInfoCategoryTable() ||
      !CreateAdInfoCategoryNameIndex())
    return false;

  // Version check.
  sql::InitStatus version_status = EnsureCurrentVersion();
  if (version_status != sql::INIT_OK)
    return version_status;

  if (!committer.Commit())
    return false;

  memory_pressure_listener_.reset(new base::MemoryPressureListener(
      base::Bind(&BundleStateDatabase::OnMemoryPressure,
      base::Unretained(this))));

  initialized_ = true;
  return initialized_;
}

bool BundleStateDatabase::CreateCategoryTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "category";
  if (GetDB().DoesTableExist(name))
    return true;

  // Note: revise implementation for InsertOrUpdatePublisherInfo() if you add
  // any new constraints to the schema.
  std::string sql;
  sql.append("CREATE TABLE ");
  sql.append(name);
  sql.append("(name LONGVARCHAR PRIMARY KEY)");
  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::CreateAdInfoTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "ad_info";
  if (GetDB().DoesTableExist(name))
    return true;

  // Update InsertOrUpdateAdInfo() if you add anything here
  std::string sql;
  sql.append("CREATE TABLE ");
  sql.append(name);
  sql.append(
      "("
      "creative_set_id LONGVARCHAR,"
      "advertiser LONGVARCHAR,"
      "notification_text TEXT,"
      "notification_url LONGVARCHAR,"
      "uuid LONGVARCHAR PRIMARY KEY)");
  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::CreateAdInfoCategoryTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "ad_info_category";
  if (GetDB().DoesTableExist(name))
    return true;

  std::string sql;
  sql.append("CREATE TABLE ");
  sql.append(name);
  sql.append(
      "("
      "ad_info_uuid LONGVARCHAR NOT NULL,"
      "category_name LONGVARCHAR NOT NULL,"
      "CONSTRAINT fk_ad_info_uuid"
      "    FOREIGN KEY (ad_info_uuid)"
      "    REFERENCES ad_info (uuid)"
      "    ON DELETE CASCADE,"
      "CONSTRAINT fk_category_name"
      "    FOREIGN KEY (category_name)"
      "    REFERENCES category (name)"
      "    ON DELETE CASCADE)");
  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::CreateAdInfoCategoryNameIndex() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return GetDB().Execute(
      "CREATE INDEX IF NOT EXISTS ad_info_category_category_name_index "
      "ON ad_info_category (category_name)");
}

bool BundleStateDatabase::SaveBundleState(
    const ads::BUNDLE_STATE& bundle_state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  // update everything inside a transaction
  return true;
}

bool BundleStateDatabase::InsertOrUpdateCategory(const std::string& category) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement ad_info_statement(
      GetDB().GetCachedStatement(SQL_FROM_HERE,
          "INSERT OR REPLACE INTO category "
          "(name) "
          "VALUES (?)"));

  ad_info_statement.BindString(0, category);

  return ad_info_statement.Run();
}

bool BundleStateDatabase::InsertOrUpdateAdInfo(const ads::AdInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement ad_info_statement(
      GetDB().GetCachedStatement(SQL_FROM_HERE,
          "INSERT OR REPLACE INTO ad_info "
          "(creative_set_id, advertiser, notification_text, "
          "notification_url, uuid) "
          "VALUES (?, ?, ?, ?, ?)"));

  ad_info_statement.BindString(0, info.creative_set_id);
  ad_info_statement.BindString(1, info.advertiser);
  ad_info_statement.BindString(2, info.notification_text);
  ad_info_statement.BindString(3, info.notification_url);
  ad_info_statement.BindString(4, info.uuid);

  return ad_info_statement.Run();
}

bool BundleStateDatabase::InsertOrUpdateAdInfoCategory(
    const ads::AdInfo& ad_info,
    const std::string& category) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement ad_info_statement(
      GetDB().GetCachedStatement(SQL_FROM_HERE,
          "INSERT OR REPLACE INTO ad_info_category "
          "(ad_info_uuid, category_name) "
          "VALUES (?, ?)"));

  ad_info_statement.BindString(0, ad_info.uuid);
  ad_info_statement.BindString(1, category);

  return ad_info_statement.Run();
}

bool BundleStateDatabase::GetAdsForCategory(const std::string& category,
                                            std::vector<ads::AdInfo>& ads) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement info_sql(
      db_.GetUniqueStatement("SELECT ai.creative_set_id, ai.advertiser, "
                             "ai.notification_text, ai.notification_url, "
                             "ai.uuid FROM ad_info AS ai "
                             "INNER JOIN ad_info_category AS aic "
                             "ON aic.ad_info_uuid = ai.uuid "
                             "WHERE aic.category_name = ?"));

  info_sql.BindString(0, category);

  if (info_sql.Step()) {
    ads::AdInfo info;
    info.creative_set_id = info_sql.ColumnString(0);
    info.advertiser = info_sql.ColumnString(1);
    info.notification_text = info_sql.ColumnString(2);
    info.notification_url = info_sql.ColumnString(3);
    info.uuid = info_sql.ColumnString(4);
    ads.emplace_back(info);
  }

  return true;
}

// static
int BundleStateDatabase::GetCurrentVersion() {
  return kCurrentVersionNumber;
}

void BundleStateDatabase::Vacuum() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!initialized_)
    return;

  DCHECK_EQ(0, db_.transaction_nesting()) <<
      "Can not have a transaction when vacuuming.";
  ignore_result(db_.Execute("VACUUM"));
}

void BundleStateDatabase::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool trim_aggressively =
      memory_pressure_level ==
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL;
  db_.TrimMemory(trim_aggressively);
}

std::string BundleStateDatabase::GetDiagnosticInfo(int extended_error,
                                               sql::Statement* statement) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(initialized_);
  return db_.GetDiagnosticInfo(extended_error, statement);
}

sql::Database& BundleStateDatabase::GetDB() {
  return db_;
}

sql::MetaTable& BundleStateDatabase::GetMetaTable() {
  return meta_table_;
}

sql::InitStatus BundleStateDatabase::EnsureCurrentVersion() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // We can't read databases newer than we were designed for.
  if (meta_table_.GetCompatibleVersionNumber() > GetCurrentVersion()) {
    LOG(WARNING) << "Publisher info database is too new.";
    return sql::INIT_TOO_NEW;
  }

  return sql::INIT_OK;
}

}  // namespace history
