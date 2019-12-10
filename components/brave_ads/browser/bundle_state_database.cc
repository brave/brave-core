/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/bundle_state_database.h"

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "build/build_config.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_ads {

namespace {

const int kCurrentVersionNumber = 2;
const int kCompatibleVersionNumber = 2;

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

  auto db_path_exists = base::PathExists(db_path_);

  if (initialized_ && db_path_exists)
    return true;

  initialized_ = false;

  if (db_.is_open()) {
    db_.Close();
    meta_table_.Reset();
  }

  if (!db_.Open(db_path_))
    return false;

  // TODO(brave): add error delegate
  sql::Transaction committer(&db_);
  if (!committer.Begin())
    return false;

  if (!meta_table_.Init(&db_, GetCurrentVersion(), kCompatibleVersionNumber))
    return false;
  if (!CreateCategoryTable() ||
      !CreateAdInfoTable() ||
      !CreateAdInfoCategoryTable() ||
      !CreateAdInfoCategoryNameIndex() ||
      !CreateConversionsTable())
    return false;

  // Version check.
  sql::InitStatus version_status = EnsureCurrentVersion();
  if (version_status != sql::INIT_OK)
    return false;

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

bool BundleStateDatabase::TruncateCategoryTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement sql(
      GetDB().GetCachedStatement(SQL_FROM_HERE, "DELETE FROM category"));
  return sql.Run();
}

bool BundleStateDatabase::CreateConversionsTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "conversions";
  if (GetDB().DoesTableExist(name))
    return true;

  std::string sql;
  sql.append("CREATE TABLE ");
  sql.append(name);
  sql.append(
      "("
      "id INTEGER PRIMARY KEY,"
      "creative_set_id LONGVARCHAR NOT NULL,"
      "type LONGVARCHAR NOT NULL,"
      "url_pattern LONGVARCHAR NOT NULL,"
      "observation_window INTEGER NOT NULL)");
  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::TruncateConversionsTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement sql(
      GetDB().GetCachedStatement(SQL_FROM_HERE,
          "DELETE FROM conversions"));
  return sql.Run();
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
      "start_timestamp DATETIME,"
      "end_timestamp DATETIME,"
      "uuid LONGVARCHAR,"
      "region VARCHAR,"
      "campaign_id LONGVARCHAR,"
      "daily_cap INTEGER DEFAULT 0 NOT NULL,"
      "per_day INTEGER DEFAULT 0 NOT NULL,"
      "total_max INTEGER DEFAULT 0 NOT NULL,"
      "PRIMARY KEY(region, uuid))");
  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::TruncateAdInfoTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement sql(
      GetDB().GetCachedStatement(SQL_FROM_HERE, "DELETE FROM ad_info"));
  return sql.Run();
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
      "UNIQUE(ad_info_uuid, category_name) ON CONFLICT REPLACE,"
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

bool BundleStateDatabase::TruncateAdInfoCategoryTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement sql(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM ad_info_category"));
  return sql.Run();
}

bool BundleStateDatabase::CreateAdInfoCategoryNameIndex() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return GetDB().Execute(
      "CREATE INDEX IF NOT EXISTS ad_info_category_category_name_index "
      "ON ad_info_category (category_name)");
}

bool BundleStateDatabase::SaveBundleState(
    const ads::BundleState& bundle_state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  if (!GetDB().BeginTransaction())
    return false;

  // we are completely replacing here so first truncate all the tables
  if (!TruncateAdInfoCategoryTable() ||
      !TruncateAdInfoTable() ||
      !TruncateCategoryTable() ||
      !TruncateConversionsTable()) {
    GetDB().RollbackTransaction();
    return false;
  }

  auto categories = bundle_state.categories;
  for (std::map<std::string, std::vector<ads::AdInfo>>::iterator it =
      categories.begin();  it != categories.end(); ++it) {
    auto category = it->first;
    if (!InsertOrUpdateCategory(category)) {
      GetDB().RollbackTransaction();
      return false;
    }

    auto ads = it->second;
    for (std::vector<ads::AdInfo>::iterator ad_it = ads.begin();
        ad_it != ads.end(); ++ad_it) {
      auto ad_info = *ad_it;
      if (!InsertOrUpdateAdInfo(ad_info) ||
          !InsertOrUpdateAdInfoCategory(ad_info, category)) {
        GetDB().RollbackTransaction();
        return false;
      }
    }
  }

  auto conversions = bundle_state.conversions;
  for (std::vector<ads::ConversionTrackingInfo>::iterator conversion_it = 
      conversions.begin(); conversion_it != conversions.end();
      ++conversion_it) {
    auto conversion_tracking_info = *conversion_it;

    if (!InsertOrUpdateConversion(conversion_tracking_info)) {
        GetDB().RollbackTransaction();
        return false;
    }

  }

  if (GetDB().CommitTransaction()) {
    Vacuum();
    return true;
  }

  return false;
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

  for (auto it = info.regions.begin(); it != info.regions.end(); ++it) {
    sql::Statement ad_info_statement(
        GetDB().GetCachedStatement(SQL_FROM_HERE,
            "INSERT OR REPLACE INTO ad_info "
            "(creative_set_id, advertiser, notification_text, "
            "notification_url, start_timestamp, end_timestamp, uuid, "
            "campaign_id, daily_cap, per_day, total_max, region) "
            "VALUES (?, ?, ?, ?, datetime(?), datetime(?), ?, ?, ?, ?, ?, ?)"));

    ad_info_statement.BindString(0, info.creative_set_id);
    ad_info_statement.BindString(1, info.advertiser);
    ad_info_statement.BindString(2, info.notification_text);
    ad_info_statement.BindString(3, info.notification_url);
    ad_info_statement.BindString(4, info.start_timestamp);
    ad_info_statement.BindString(5, info.end_timestamp);
    ad_info_statement.BindString(6, info.uuid);
    ad_info_statement.BindString(7, info.campaign_id);
    ad_info_statement.BindInt(8, info.daily_cap);
    ad_info_statement.BindInt(9, info.per_day);
    ad_info_statement.BindInt(10, info.total_max);
    ad_info_statement.BindString(11, *it);
    if (!ad_info_statement.Run()) {
      return false;
    }
  }

  return true;
}

bool BundleStateDatabase::InsertOrUpdateConversion(
    const ads::ConversionTrackingInfo& conversion_tracking_info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement conversion_info_statement(
      GetDB().GetCachedStatement(SQL_FROM_HERE,
          "INSERT OR REPLACE INTO conversions "
          "(creative_set_id, type, url_pattern, observation_window) "
          "VALUES (?, ?, ?, ?)"));

  conversion_info_statement.BindString(0,
      conversion_tracking_info.creative_set_id);
  conversion_info_statement.BindString(1,
      conversion_tracking_info.type);
  conversion_info_statement.BindString(2,
      conversion_tracking_info.url_pattern);
  conversion_info_statement.BindInt(3,
      conversion_tracking_info.observation_window);

  return conversion_info_statement.Run();
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

bool BundleStateDatabase::GetAdsForCategory(
    const std::string& category,
    std::vector<ads::AdInfo>* ads) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement info_sql(
      db_.GetUniqueStatement(
          "SELECT ai.creative_set_id, ai.advertiser, "
          "ai.notification_text, ai.notification_url, "
          "ai.start_timestamp, ai.end_timestamp, "
          "ai.uuid, ai.region, ai.campaign_id, ai.daily_cap, "
          "ai.per_day, ai.total_max FROM ad_info AS ai "
          "INNER JOIN ad_info_category AS aic "
          "ON aic.ad_info_uuid = ai.uuid "
          "WHERE aic.category_name = ? and "
          "ai.start_timestamp <= strftime('%Y-%m-%d %H:%M', "
          "datetime('now','localtime')) and "
          "ai.end_timestamp >= strftime('%Y-%m-%d %H:%M', "
          "datetime('now','localtime'));"));
  info_sql.BindString(0, category);

  while (info_sql.Step()) {
    ads::AdInfo info;
    info.creative_set_id = info_sql.ColumnString(0);
    info.advertiser = info_sql.ColumnString(1);
    info.category = category;
    info.notification_text = info_sql.ColumnString(2);
    info.notification_url = info_sql.ColumnString(3);
    info.start_timestamp = info_sql.ColumnString(4);
    info.end_timestamp = info_sql.ColumnString(5);
    info.uuid = info_sql.ColumnString(6);
    info.campaign_id = info_sql.ColumnString(8);
    info.daily_cap = info_sql.ColumnInt(9);
    info.per_day = info_sql.ColumnInt(10);
    info.total_max = info_sql.ColumnInt(11);
    ads->emplace_back(info);
  }

  return true;
}

bool BundleStateDatabase::GetConversions(
    const std::string& url,
    std::vector<ads::ConversionTrackingInfo>* conversions) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement info_sql(
      db_.GetUniqueStatement(
          "SELECT c.creative_set_id, c.type, c.url_pattern, "
          "c.observation_window "
          "FROM conversions AS c"));

  while (info_sql.Step()) {
    ads::ConversionTrackingInfo info;
    info.creative_set_id = info_sql.ColumnString(0);
    info.type = info_sql.ColumnString(1);
    info.url_pattern = info_sql.ColumnString(2);
    info.observation_window = info_sql.ColumnInt(3);
    conversions->emplace_back(info);
  }

  return true;
}

bool BundleStateDatabase::SaveConversion(
    const ads::ConversionTrackingInfo& conversion) {
  LOG(INFO) << "*** Save Conversion";
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
  db_.TrimMemory();
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

bool BundleStateDatabase::MigrateV1toV2() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!GetDB().BeginTransaction())
    return false;

  std::string sql = "ALTER TABLE ad_info ADD campaign_id LONGVARCHAR;";
  if (!GetDB().Execute(sql.c_str())) {
    GetDB().RollbackTransaction();
    return false;
  }

  sql = "ALTER TABLE ad_info ADD daily_cap INTEGER DEFAULT 0 NOT NULL;";
  if (!GetDB().Execute(sql.c_str())) {
    GetDB().RollbackTransaction();
    return false;
  }

  sql = "ALTER TABLE ad_info ADD per_day INTEGER DEFAULT 0 NOT NULL;";
  if (!GetDB().Execute(sql.c_str())) {
    GetDB().RollbackTransaction();
    return false;
  }

  sql = "ALTER TABLE ad_info ADD total_max INTEGER DEFAULT 0 NOT NULL;";
  if (!GetDB().Execute(sql.c_str())) {
    GetDB().RollbackTransaction();
    return false;
  }

  if (GetDB().CommitTransaction()) {
    Vacuum();
    return true;
  }

  return false;
}

sql::InitStatus BundleStateDatabase::EnsureCurrentVersion() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // We can't read databases newer than we were designed for.
  if (meta_table_.GetCompatibleVersionNumber() > GetCurrentVersion()) {
    LOG(WARNING) << "Publisher info database is too new.";
    return sql::INIT_TOO_NEW;
  }

  const int old_version = meta_table_.GetVersionNumber();
  const int cur_version = GetCurrentVersion();

  // Migration from version 1 to version 2
  if (old_version == 1 && cur_version == 2) {
    if (!MigrateV1toV2()) {
      LOG(ERROR) << "DB: Error with MigrateV1toV2";
    }

    meta_table_.SetVersionNumber(cur_version);
  }

  return sql::INIT_OK;
}

}  // namespace brave_ads
