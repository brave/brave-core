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
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_ads {

namespace {

const int kCurrentVersionNumber = 4;
const int kCompatibleVersionNumber = 4;

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
      !CreateCreativeAdNotificationInfoTable() ||
      !CreateCreativeAdNotificationInfoCategoryTable() ||
      !CreateCreativeAdNotificationInfoCategoryNameIndex() ||
      !CreateConversionsTable())
    return false;

  if (!Migrate())
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

  const char* name = "ad_conversions";
  if (GetDB().DoesTableExist(name)) {
    return true;
  }

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

  const bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  sql::Statement sql(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM ad_conversions"));
  return sql.Run();
}

bool BundleStateDatabase::CreateCreativeAdNotificationInfoTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "ad_info";
  if (GetDB().DoesTableExist(name))
    return true;

  // Update InsertOrUpdateCreativeAdNotificationInfo() if you add anything here
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
      "advertiser_id LONGVARCHAR,"
      "per_day INTEGER DEFAULT 0 NOT NULL,"
      "total_max INTEGER DEFAULT 0 NOT NULL,"
      "PRIMARY KEY(region, uuid))");
  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::TruncateCreativeAdNotificationInfoTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement sql(
      GetDB().GetCachedStatement(SQL_FROM_HERE, "DELETE FROM ad_info"));
  return sql.Run();
}

bool BundleStateDatabase::CreateCreativeAdNotificationInfoCategoryTable() {
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

bool BundleStateDatabase::TruncateCreativeAdNotificationInfoCategoryTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  sql::Statement sql(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM ad_info_category"));
  return sql.Run();
}

bool BundleStateDatabase::CreateCreativeAdNotificationInfoCategoryNameIndex() {
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
  if (!TruncateCreativeAdNotificationInfoCategoryTable() ||
      !TruncateCreativeAdNotificationInfoTable() ||
      !TruncateCategoryTable() ||
      !TruncateConversionsTable()) {
    GetDB().RollbackTransaction();
    return false;
  }

  auto categories = bundle_state.creative_ad_notifications;
  for (auto it = categories.begin(); it != categories.end(); ++it) {
    auto category = it->first;
    if (!InsertOrUpdateCategory(category)) {
      GetDB().RollbackTransaction();
      return false;
    }

    auto ads = it->second;
    for (auto it = ads.begin(); it != ads.end(); ++it) {
      auto ad_info = *it;
      if (!InsertOrUpdateCreativeAdNotificationInfo(ad_info) ||
          !InsertOrUpdateCreativeAdNotificationInfoCategory(
              ad_info, category)) {
        GetDB().RollbackTransaction();
        return false;
      }
    }
  }

  for (const auto& ad_conversion : bundle_state.ad_conversions) {
    if (!InsertOrUpdateAdConversion(ad_conversion)) {
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

bool BundleStateDatabase::InsertOrUpdateCreativeAdNotificationInfo(
    const ads::CreativeAdNotificationInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized)
    return false;

  for (auto it = info.geo_targets.begin(); it != info.geo_targets.end(); ++it) {
    sql::Statement ad_info_statement(
        GetDB().GetCachedStatement(SQL_FROM_HERE,
            "INSERT OR REPLACE INTO ad_info "
            "(creative_set_id, advertiser, notification_text, "
            "notification_url, start_timestamp, end_timestamp, uuid, "
            "campaign_id, daily_cap, advertiser_id, per_day, total_max, "
            "region) VALUES (?, ?, ?, ?, datetime(?), datetime(?), ?, ?, ?, "
            "?, ?, ?, ?)"));

    ad_info_statement.BindString(0, info.creative_set_id);
    ad_info_statement.BindString(1, info.title);
    ad_info_statement.BindString(2, info.body);
    ad_info_statement.BindString(3, info.target_url);
    ad_info_statement.BindString(4, info.start_at_timestamp);
    ad_info_statement.BindString(5, info.end_at_timestamp);
    ad_info_statement.BindString(6, info.creative_instance_id);
    ad_info_statement.BindString(7, info.campaign_id);
    ad_info_statement.BindInt(8, info.daily_cap);
    ad_info_statement.BindString(9, info.advertiser_id);
    ad_info_statement.BindInt(10, info.per_day);
    ad_info_statement.BindInt(11, info.total_max);
    ad_info_statement.BindString(12, *it);
    if (!ad_info_statement.Run()) {
      return false;
    }
  }

  return true;
}

bool BundleStateDatabase::InsertOrUpdateAdConversion(
    const ads::AdConversionInfo& ad_conversion) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  sql::Statement statement(
      GetDB().GetCachedStatement(SQL_FROM_HERE,
          "INSERT OR REPLACE INTO ad_conversions "
          "(creative_set_id, type, url_pattern, observation_window) "
          "VALUES (?, ?, ?, ?)"));

  statement.BindString(0, ad_conversion.creative_set_id);
  statement.BindString(1, ad_conversion.type);
  statement.BindString(2, ad_conversion.url_pattern);
  statement.BindInt(3, ad_conversion.observation_window);

  return statement.Run();
}

bool BundleStateDatabase::InsertOrUpdateCreativeAdNotificationInfoCategory(
    const ads::CreativeAdNotificationInfo& info,
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

  ad_info_statement.BindString(0, info.creative_instance_id);
  ad_info_statement.BindString(1, category);

  return ad_info_statement.Run();
}

bool BundleStateDatabase::GetCreativeAdNotifications(
    const std::vector<std::string>& categories,
    ads::CreativeAdNotificationList* ads) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(ads);
  if (!ads) {
    return false;
  }

  bool initialized = Init();
  DCHECK(initialized);
  if (!initialized) {
    return false;
  }

  sql::Statement info_sql(
      db_.GetUniqueStatement(
          "SELECT ai.creative_set_id, ai.advertiser, "
          "ai.notification_text, ai.notification_url, "
          "ai.start_timestamp, ai.end_timestamp, "
          "ai.uuid, ai.region, ai.campaign_id, ai.daily_cap, "
          "ai.advertiser_id, ai.per_day, ai.total_max, aic.category_name "
          "FROM ad_info AS ai "
          "INNER JOIN ad_info_category AS aic "
          "ON aic.ad_info_uuid = ai.uuid "
          "WHERE aic.category_name IN (?) and "
          "ai.start_timestamp <= strftime('%Y-%m-%d %H:%M', "
          "datetime('now','localtime')) and "
          "ai.end_timestamp >= strftime('%Y-%m-%d %H:%M', "
          "datetime('now','localtime'));"));
  std::vector<std::string> quoted_categories;
  for (const auto& category : categories) {
    const std::string quoted_category = base::StringPrintf("\"%s\"",
        category.c_str());
    quoted_categories.push_back(quoted_category);
  }
  info_sql.BindString(0, base::JoinString(quoted_categories, ", ").c_str());

  while (info_sql.Step()) {
    ads::CreativeAdNotificationInfo info;
    info.creative_set_id = info_sql.ColumnString(0);
    info.title = info_sql.ColumnString(1);
    info.body = info_sql.ColumnString(2);
    info.target_url = info_sql.ColumnString(3);
    info.start_at_timestamp = info_sql.ColumnString(4);
    info.end_at_timestamp = info_sql.ColumnString(5);
    info.creative_instance_id = info_sql.ColumnString(6);
    info.geo_targets.push_back(info_sql.ColumnString(7));
    info.campaign_id = info_sql.ColumnString(8);
    info.daily_cap = info_sql.ColumnInt(9);
    info.advertiser_id = info_sql.ColumnString(10);
    info.per_day = info_sql.ColumnInt(11);
    info.total_max = info_sql.ColumnInt(12);
    info.category = info_sql.ColumnInt(13);
    ads->emplace_back(info);
  }

  return true;
}

bool BundleStateDatabase::GetAdConversions(
    const std::string& url,
    ads::AdConversionList* ad_conversions) {
  DCHECK(ad_conversions);
  if (!ad_conversions) {
    return false;
  }

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  sql::Statement info_sql(
      db_.GetUniqueStatement(
          "SELECT c.creative_set_id, c.type, c.url_pattern, "
          "c.observation_window "
          "FROM ad_conversions AS c"));

  while (info_sql.Step()) {
    ads::AdConversionInfo ad_conversion;
    ad_conversion.creative_set_id = info_sql.ColumnString(0);
    ad_conversion.type = info_sql.ColumnString(1);
    ad_conversion.url_pattern = info_sql.ColumnString(2);
    ad_conversion.observation_window = info_sql.ColumnInt(3);
    ad_conversions->emplace_back(ad_conversion);
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

bool BundleStateDatabase::Migrate() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!GetDB().BeginTransaction()) {
    return false;
  }

  // We can't read databases newer than we were designed for.
  if (meta_table_.GetCompatibleVersionNumber() > GetCurrentVersion()) {
    LOG(WARNING) << "Bundle state database is too new";
    return false;
  }

  const int source_version = meta_table_.GetVersionNumber();
  const int dest_version = GetCurrentVersion();

  // Migration database
  for (int i = source_version; i < dest_version; i++) {
    switch (i) {
      case 1: {
        if (!MigrateV1toV2()) {
          LOG(ERROR) << "DB: Error migrating database from v1 to v2";
          return false;
        }

        break;
      }

      case 2: {
        if (!MigrateV2toV3()) {
          LOG(ERROR) << "DB: Error migrating database from v2 to v3";
          return false;
        }

        break;
      }

      case 3: {
        if (!MigrateV3toV4()) {
          LOG(ERROR) << "DB: Error migrating database from v3 to v4";
          return false;
        }

        break;
      }

      default: {
        NOTREACHED();
        return false;
      }
    }
  }

  meta_table_.SetVersionNumber(dest_version);

  if (GetDB().CommitTransaction()) {
    Vacuum();
    return true;
  }

  return false;
}

bool BundleStateDatabase::MigrateV1toV2() {
  std::string sql = "ALTER TABLE ad_info ADD campaign_id LONGVARCHAR;";
  if (!GetDB().Execute(sql.c_str())) {
    return false;
  }

  sql = "ALTER TABLE ad_info ADD daily_cap INTEGER DEFAULT 0 NOT NULL;";
  if (!GetDB().Execute(sql.c_str())) {
    return false;
  }

  sql = "ALTER TABLE ad_info ADD per_day INTEGER DEFAULT 0 NOT NULL;";
  if (!GetDB().Execute(sql.c_str())) {
    return false;
  }

  sql = "ALTER TABLE ad_info ADD total_max INTEGER DEFAULT 0 NOT NULL;";
  if (!GetDB().Execute(sql.c_str())) {
    return false;
  }

  return true;
}

bool BundleStateDatabase::MigrateV2toV3() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "ad_conversions";
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

bool BundleStateDatabase::MigrateV3toV4() {
  std::string sql = "ALTER TABLE ad_info ADD advertiser_id LONGVARCHAR;";
  return GetDB().Execute(sql.c_str());
}

}  // namespace brave_ads
