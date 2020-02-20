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

BundleStateDatabase::BundleStateDatabase(
    const base::FilePath& db_path)
    : db_path_(db_path),
      is_initialized_(false) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

BundleStateDatabase::~BundleStateDatabase() = default;

bool BundleStateDatabase::Init() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto db_path_exists = base::PathExists(db_path_);

  if (is_initialized_ && db_path_exists) {
    return true;
  }

  is_initialized_ = false;

  if (db_.is_open()) {
    db_.Close();
    meta_table_.Reset();
  }

  if (!db_.Open(db_path_)) {
    return false;
  }

  // TODO(brave): add error delegate
  sql::Transaction committer(&db_);
  if (!committer.Begin()) {
    return false;
  }

  if (!meta_table_.Init(&db_, GetCurrentVersion(), kCompatibleVersionNumber)) {
    return false;
  }

  if (!CreateCategoryTable() ||
      !CreateCreativeAdNotificationInfoTable() ||
      !CreateCreativeAdNotificationInfoCategoryTable() ||
      !CreateCreativeAdNotificationInfoCategoryNameIndex() ||
      !CreateAdConversionsTable()) {
    return false;
  }

  if (!Migrate()) {
    return false;
  }

  if (!committer.Commit()) {
    return false;
  }

  memory_pressure_listener_.reset(new base::MemoryPressureListener(
      base::Bind(&BundleStateDatabase::OnMemoryPressure,
          base::Unretained(this))));

  is_initialized_ = true;
  return is_initialized_;
}

bool BundleStateDatabase::CreateCategoryTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char name[] = "category";
  if (GetDB().DoesTableExist(name)) {
    return true;
  }

  // Note: revise implementation for InsertOrUpdateCategory() if you add
  // any new constraints to the schema.
  std::string sql;
  sql.append("CREATE TABLE ");
  sql.append(name);
  sql.append("(name LONGVARCHAR PRIMARY KEY)");

  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::TruncateCategoryTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);
  if (!is_initialized) {
    return false;
  }

  sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM category"));

  return statement.Run();
}

bool BundleStateDatabase::CreateAdConversionsTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char name[] = "ad_conversions";
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

bool BundleStateDatabase::TruncateAdConversionsTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  if (!is_initialized) {
    return false;
  }

  sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM ad_conversions"));

  return statement.Run();
}

bool BundleStateDatabase::CreateCreativeAdNotificationInfoTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char name[] = "ad_info";
  if (GetDB().DoesTableExist(name)) {
    return true;
  }

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

  const bool is_initialized = Init();
  DCHECK(is_initialized);
  if (!is_initialized) {
    return false;
  }

  sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM ad_info"));

  return statement.Run();
}

bool BundleStateDatabase::CreateCreativeAdNotificationInfoCategoryTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "ad_info_category";
  if (GetDB().DoesTableExist(name)) {
    return true;
  }

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

  const bool is_initialized = Init();
  DCHECK(is_initialized);
  if (!is_initialized) {
    return false;
  }

  sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM ad_info_category"));
  return statement.Run();
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

  const bool is_initialized = Init();
  DCHECK(is_initialized);
  if (!is_initialized) {
    return false;
  }

  if (!GetDB().BeginTransaction()) {
    return false;
  }

  // We are completely replacing here so first truncate all the tables
  if (!TruncateCategoryTable() ||
      !TruncateCreativeAdNotificationInfoCategoryTable() ||
      !TruncateCreativeAdNotificationInfoTable() ||
      !TruncateAdConversionsTable()) {
    GetDB().RollbackTransaction();
    return false;
  }

  for (const auto& creative_ad_notification :
      bundle_state.creative_ad_notifications) {
    const std::string category_name = creative_ad_notification.first;
    if (!InsertOrUpdateCategory(category_name)) {
      GetDB().RollbackTransaction();
      return false;
    }

    const ads::CreativeAdNotificationList ads = creative_ad_notification.second;
    for (const auto& ad : ads) {
      if (!InsertOrUpdateCreativeAdNotificationInfo(ad) ||
          !InsertOrUpdateCreativeAdNotificationInfoCategory(ad,
              category_name)) {
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

  if (!GetDB().CommitTransaction()) {
    return false;
  }

  Vacuum();
  return true;
}

bool BundleStateDatabase::InsertOrUpdateCategory(
    const std::string& category) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);
  if (!is_initialized) {
    return false;
  }

  sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "INSERT OR REPLACE INTO category (name) VALUES (?)"));

  statement.BindString(0, category);

  return statement.Run();
}

bool BundleStateDatabase::InsertOrUpdateCreativeAdNotificationInfo(
    const ads::CreativeAdNotificationInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool is_initialized = Init();
  DCHECK(is_initialized);
  if (!is_initialized) {
    return false;
  }

  for (const auto& geo_target : info.geo_targets) {
    sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE,
        "INSERT OR REPLACE INTO ad_info "
        "(creative_set_id, advertiser, notification_text, "
        "notification_url, start_timestamp, end_timestamp, uuid, "
        "campaign_id, daily_cap, advertiser_id, per_day, total_max, "
        "region) VALUES (?, ?, ?, ?, datetime(?), datetime(?), ?, ?, ?, "
        "?, ?, ?, ?)"));

    statement.BindString(0, info.creative_set_id);
    statement.BindString(1, info.title);
    statement.BindString(2, info.body);
    statement.BindString(3, info.target_url);
    statement.BindString(4, info.start_at_timestamp);
    statement.BindString(5, info.end_at_timestamp);
    statement.BindString(6, info.creative_instance_id);
    statement.BindString(7, info.campaign_id);
    statement.BindInt(8, info.daily_cap);
    statement.BindString(9, info.advertiser_id);
    statement.BindInt(10, info.per_day);
    statement.BindInt(11, info.total_max);
    statement.BindString(12, geo_target);

    if (!statement.Run()) {
      return false;
    }
  }

  return true;
}

bool BundleStateDatabase::InsertOrUpdateAdConversion(
    const ads::AdConversionInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);
  if (!is_initialized) {
    return false;
  }

  sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "INSERT OR REPLACE INTO ad_conversions "
      "(creative_set_id, type, url_pattern, observation_window) "
      "VALUES (?, ?, ?, ?)"));

  statement.BindString(0, info.creative_set_id);
  statement.BindString(1, info.type);
  statement.BindString(2, info.url_pattern);
  statement.BindInt(3, info.observation_window);

  return statement.Run();
}

bool BundleStateDatabase::InsertOrUpdateCreativeAdNotificationInfoCategory(
    const ads::CreativeAdNotificationInfo& info,
    const std::string& category) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);
  if (!is_initialized) {
    return false;
  }

  sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "INSERT OR REPLACE INTO ad_info_category "
      "(ad_info_uuid, category_name) "
      "VALUES (?, ?)"));

  statement.BindString(0, info.creative_instance_id);
  statement.BindString(1, category);

  return statement.Run();
}

bool BundleStateDatabase::GetCreativeAdNotifications(
    const std::vector<std::string>& categories,
    ads::CreativeAdNotificationList* ads) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(ads);
  if (!ads) {
    return false;
  }

  const bool is_initialized = Init();
  DCHECK(is_initialized);
  if (!is_initialized) {
    return false;
  }

  std::string sql =
      "SELECT ai.creative_set_id, "
      "ai.advertiser, "
      "ai.notification_text, "
      "ai.notification_url, "
      "ai.start_timestamp, "
      "ai.end_timestamp, "
      "ai.uuid, "
      "ai.region, "
      "ai.campaign_id, "
      "ai.daily_cap, "
      "ai.advertiser_id, "
      "ai.per_day, "
      "ai.total_max, "
      "aic.category_name "
      "FROM ad_info AS ai "
      "INNER JOIN ad_info_category AS aic "
      "ON aic.ad_info_uuid = ai.uuid "
      "WHERE ";

  sql += "aic.category_name IN (";
  for (size_t i = 0; i < categories.size(); i++) {
    if (i != 0) {
      sql += ", ";
    }

    sql += "?";
  }
  sql += ") and ";

  sql +=
      "ai.start_timestamp <= strftime('%Y-%m-%d %H:%M', "
      "datetime('now','localtime')) and "
      "ai.end_timestamp >= strftime('%Y-%m-%d %H:%M', "
      "datetime('now','localtime'));";

  sql::Statement statement(db_.GetUniqueStatement(sql.c_str()));

  int index = 0;
  for (const auto& category : categories) {
    statement.BindString(index, category.c_str());
    index++;
  }

  while (statement.Step()) {
    ads::CreativeAdNotificationInfo info;
    info.creative_set_id = statement.ColumnString(0);
    info.title = statement.ColumnString(1);
    info.body = statement.ColumnString(2);
    info.target_url = statement.ColumnString(3);
    info.start_at_timestamp = statement.ColumnString(4);
    info.end_at_timestamp = statement.ColumnString(5);
    info.creative_instance_id = statement.ColumnString(6);
    info.geo_targets.push_back(statement.ColumnString(7));
    info.campaign_id = statement.ColumnString(8);
    info.daily_cap = statement.ColumnInt(9);
    info.advertiser_id = statement.ColumnString(10);
    info.per_day = statement.ColumnInt(11);
    info.total_max = statement.ColumnInt(12);
    info.category = statement.ColumnInt(13);
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

  const bool is_initialized = Init();
  DCHECK(is_initialized);
  if (!is_initialized) {
    return false;
  }

  sql::Statement statement(db_.GetUniqueStatement(
      "SELECT c.creative_set_id, c.type, c.url_pattern, "
      "c.observation_window "
      "FROM ad_conversions AS c"));

  while (statement.Step()) {
    ads::AdConversionInfo info;
    info.creative_set_id = statement.ColumnString(0);
    info.type = statement.ColumnString(1);
    info.url_pattern = statement.ColumnString(2);
    info.observation_window = statement.ColumnInt(3);
    ad_conversions->emplace_back(info);
  }

  return true;
}

// static
int BundleStateDatabase::GetCurrentVersion() {
  return kCurrentVersionNumber;
}

void BundleStateDatabase::Vacuum() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!is_initialized_) {
    return;
  }

  DCHECK_EQ(0, db_.transaction_nesting()) <<
      "Can not have a transaction when vacuuming.";
  ignore_result(db_.Execute("VACUUM"));
}

void BundleStateDatabase::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  db_.TrimMemory();
}

std::string BundleStateDatabase::GetDiagnosticInfo(
    const int extended_error,
    sql::Statement* statement) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(is_initialized_);
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

  // Migrate database
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

  if (!GetDB().CommitTransaction()) {
    return false;
  }

  Vacuum();
  return true;
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

bool BundleStateDatabase::MigrateV3toV4() {
  std::string sql = "ALTER TABLE ad_info ADD advertiser_id LONGVARCHAR;";

  return GetDB().Execute(sql.c_str());
}

}  // namespace brave_ads
