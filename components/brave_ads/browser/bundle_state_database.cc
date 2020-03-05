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
#include "url/gurl.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace brave_ads {

namespace {

const int kCurrentVersionNumber = 5;
const int kCompatibleVersionNumber = 5;

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

  if (!CreateCategoriesTable() ||
      !CreateCreativeAdNotificationsTable() ||
      !CreateCreativeAdNotificationCategoriesTable() ||
      !CreateCreativeAdNotificationCategoriesCategoryIndex() ||
      !CreateCreativePublisherAdsTable() ||
      !CreateCreativePublisherAdsCategoriesTable() ||
      !CreateCreativePublisherAdsCategoriesCategoryIndex() ||
      !CreateCreativePublisherAdsChannelsTable() ||
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

bool BundleStateDatabase::CreateCategoriesTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char table_name[] = "category";
  if (GetDB().DoesTableExist(table_name)) {
    return true;
  }

  // Note: revise implementation for |InsertOrUpdateCategory| if you add any new
  // constraints to the schema
  const std::string sql = base::StringPrintf(
      "CREATE TABLE %s "
          "(name LONGVARCHAR PRIMARY KEY)",
      table_name);

  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::TruncateCategoriesTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql =
      "DELETE FROM category";

  sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

  return statement.Run();
}

bool BundleStateDatabase::InsertOrUpdateCategory(
    const std::string& category) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql = base::StringPrintf(
      "INSERT OR REPLACE INTO category "
          "(name) VALUES (%s)",
      CreateBindingParameterPlaceholders(1).c_str());

  sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

  statement.BindString(0, category);

  return statement.Run();
}

bool BundleStateDatabase::CreateCreativeAdNotificationsTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char table_name[] = "ad_info";
  if (GetDB().DoesTableExist(table_name)) {
    return true;
  }

  // Note: revise implementation for |InsertOrUpdateCreativeAdNotification| if
  // you add any new constraints to the schema
  const std::string sql = base::StringPrintf(
      "CREATE TABLE %s "
          "(creative_set_id LONGVARCHAR, "
          "advertiser LONGVARCHAR, "
          "notification_text TEXT, "
          "notification_url LONGVARCHAR, "
          "start_timestamp DATETIME, "
          "end_timestamp DATETIME, "
          "uuid LONGVARCHAR, "
          "region VARCHAR, "
          "campaign_id LONGVARCHAR, "
          "daily_cap INTEGER DEFAULT 0 NOT NULL, "
          "advertiser_id LONGVARCHAR, "
          "per_day INTEGER DEFAULT 0 NOT NULL, "
          "total_max INTEGER DEFAULT 0 NOT NULL, "
          "PRIMARY KEY(region, uuid))",
      table_name);

  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::TruncateCreativeAdNotificationsTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql =
      "DELETE FROM ad_info";

  sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

  return statement.Run();
}

bool BundleStateDatabase::InsertOrUpdateCreativeAdNotification(
    const ads::CreativeAdNotificationInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  for (const auto& geo_target : info.geo_targets) {
    const std::string sql = base::StringPrintf(
        "INSERT OR REPLACE INTO ad_info "
            "(creative_set_id, "
            "advertiser, "
            "notification_text, "
            "notification_url, "
            "start_timestamp, "
            "end_timestamp, "
            "uuid, "
            "campaign_id, "
            "daily_cap, "
            "advertiser_id, "
            "per_day, "
            "total_max, "
            "region) VALUES (%s)",
      CreateBindingParameterPlaceholders(13).c_str());

    sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

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

bool BundleStateDatabase::CreateCreativeAdNotificationCategoriesTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char table_name[] = "ad_info_category";
  if (GetDB().DoesTableExist(table_name)) {
    return true;
  }

  const std::string sql = base::StringPrintf(
      "CREATE TABLE %s "
          "(ad_info_uuid LONGVARCHAR NOT NULL, "
          "category_name LONGVARCHAR NOT NULL, "
          "UNIQUE(ad_info_uuid, category_name) ON CONFLICT REPLACE, "
          "CONSTRAINT fk_ad_info_uuid "
              "FOREIGN KEY (ad_info_uuid) "
              "REFERENCES ad_info (uuid) "
              "ON DELETE CASCADE, "
          "CONSTRAINT fk_category_name "
              "FOREIGN KEY (category_name) "
              "REFERENCES category (name) "
              "ON DELETE CASCADE)",
      table_name);

  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::TruncateCreativeAdNotificationCategoriesTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql =
      "DELETE FROM ad_info_category";

  sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

  return statement.Run();
}

bool BundleStateDatabase::InsertOrUpdateCreativeAdNotificationCategory(
    const ads::CreativeAdNotificationInfo& info,
    const std::string& category) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql = base::StringPrintf(
      "INSERT OR REPLACE INTO ad_info_category "
          "(ad_info_uuid, "
          "category_name) VALUES (%s)",
      CreateBindingParameterPlaceholders(2).c_str());

  sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

  statement.BindString(0, info.creative_instance_id);
  statement.BindString(1, category);

  return statement.Run();
}

bool
BundleStateDatabase::CreateCreativeAdNotificationCategoriesCategoryIndex() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const std::string sql =
      "CREATE INDEX IF NOT EXISTS ad_info_category_category_name_index "
          "ON ad_info_category (category_name)";

  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::CreateCreativePublisherAdsTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char table_name[] = "creative_publisher_ads";
  if (GetDB().DoesTableExist(table_name)) {
    return true;
  }

  // Note: revise implementation for |InsertOrUpdateCreativePublisherAd| if you
  // add any new constraints to the schema
  const std::string sql = base::StringPrintf(
      "CREATE TABLE %s "
          "(creative_instance_id LONGVARCHAR, "
          "creative_set_id LONGVARCHAR, "
          "campaign_id LONGVARCHAR, "
          "start_at_timestamp DATETIME, "
          "end_at_timestamp DATETIME, "
          "daily_cap INTEGER DEFAULT 0 NOT NULL, "
          "advertiser_id LONGVARCHAR, "
          "per_day INTEGER DEFAULT 0 NOT NULL, "
          "total_max INTEGER DEFAULT 0 NOT NULL, "
          "geo_target VARCHAR, "
          "size TEXT, "
          "creative_url LONGVARCHAR, "
          "target_url LONGVARCHAR, "
          "channel VARCHAR, "
          "PRIMARY KEY (creative_instance_id, geo_target, channel))",
      table_name);

  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::TruncateCreativePublisherAdsTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql =
      "DELETE FROM creative_publisher_ads";

  sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

  return statement.Run();
}

bool BundleStateDatabase::InsertOrUpdateCreativePublisherAd(
    const ads::CreativePublisherAdInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  for (const auto& channel : info.channels) {
    for (const auto& geo_target : info.geo_targets) {
      const std::string sql = base::StringPrintf(
          "INSERT OR REPLACE INTO creative_publisher_ads "
             "(creative_instance_id, "
             "creative_set_id, "
             "campaign_id, "
             "start_at_timestamp, "
             "end_at_timestamp, "
             "daily_cap, "
             "advertiser_id, "
             "per_day, "
             "total_max, "
             "geo_target, "
             "size, "
             "creative_url, "
             "target_url, "
             "channel) VALUES (%s)",
          CreateBindingParameterPlaceholders(14).c_str());

      sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

      statement.BindString(0, info.creative_instance_id);
      statement.BindString(1, info.creative_set_id);
      statement.BindString(2, info.campaign_id);
      statement.BindString(3, info.start_at_timestamp);
      statement.BindString(4, info.end_at_timestamp);
      statement.BindInt(5, info.daily_cap);
      statement.BindString(6, info.advertiser_id);
      statement.BindInt(7, info.per_day);
      statement.BindInt(8, info.total_max);
      statement.BindString(9, geo_target);
      statement.BindString(10, info.size);
      statement.BindString(11, info.creative_url);
      statement.BindString(12, info.target_url);
      statement.BindString(13, channel);

      if (!statement.Run()) {
        return false;
      }
    }
  }

  return true;
}

bool BundleStateDatabase::CreateCreativePublisherAdsCategoriesTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char table_name[] = "creative_publisher_ads_categories";
  if (GetDB().DoesTableExist(table_name)) {
    return true;
  }

  const std::string sql = base::StringPrintf(
      "CREATE TABLE %s "
          "(creative_instance_id LONGVARCHAR NOT NULL, "
          "category LONGVARCHAR NOT NULL, "
          "UNIQUE(creative_instance_id, category) ON CONFLICT REPLACE, "
          "CONSTRAINT fk_creative_instance_id "
              "FOREIGN KEY (creative_instance_id) "
              "REFERENCES creative_publisher_ads (creative_instance_id) "
              "ON DELETE CASCADE, "
          "CONSTRAINT fk_category "
              "FOREIGN KEY (category) "
              "REFERENCES category (category) "
              "ON DELETE CASCADE)",
      table_name);

  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::TruncateCreativePublisherAdsCategoriesTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql =
      "DELETE FROM creative_publisher_ads_categories";

  sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

  return statement.Run();
}

bool BundleStateDatabase::InsertOrUpdateCreativePublisherAdCategory(
    const ads::CreativePublisherAdInfo& info,
    const std::string& category) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql = base::StringPrintf(
      "INSERT OR REPLACE INTO creative_publisher_ads_categories "
          "(creative_instance_id, "
          "category) VALUES (%s)",
      CreateBindingParameterPlaceholders(2).c_str());

  sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

  statement.BindString(0, info.creative_instance_id);
  statement.BindString(1, category);

  return statement.Run();
}

bool BundleStateDatabase::CreateCreativePublisherAdsCategoriesCategoryIndex() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const std::string sql =
      "CREATE INDEX IF NOT EXISTS "
          "creative_publisher_ads_categories_category_index "
              "ON creative_publisher_ads_categories (category)";

  return GetDB().Execute(sql.c_str());
}


bool BundleStateDatabase::CreateCreativePublisherAdsChannelsTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char table_name[] = "creative_publisher_ads_channels";
  if (GetDB().DoesTableExist(table_name)) {
    return true;
  }

  // Note: revise implementation for |InsertOrUpdateCreativePublisherAdChannel|
  // if you add any new constraints to the schema
  const std::string sql = base::StringPrintf(
      "CREATE TABLE %s "
          "(channel LONGVARCHAR PRIMARY KEY)", table_name);

  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::TruncateCreativePublisherAdsChannelsTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql =
      "DELETE FROM creative_publisher_ads_channels";

  sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

  return statement.Run();
}

bool BundleStateDatabase::InsertOrUpdateCreativePublisherAdChannel(
    const ads::CreativePublisherAdInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  if (info.channels.empty()) {
    return true;
  }

  std::string parameters;
  for (size_t i = 0; i < info.channels.size() - 1; i++) {
    parameters += "(?), ";
  }
  parameters += "(?)";

  const std::string sql = base::StringPrintf(
      "INSERT OR REPLACE INTO creative_publisher_ads_channels "
          "(channel) VALUES %s",
      parameters.c_str());

  sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

  int index = 0;
  for (const auto& channel : info.channels) {
    statement.BindString(index, channel.c_str());
    index++;
  }

  return statement.Run();
}

bool BundleStateDatabase::CreateAdConversionsTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char table_name[] = "ad_conversions";
  if (GetDB().DoesTableExist(table_name)) {
    return true;
  }

  // Note: revise implementation for |InsertOrUpdateAdConversion| if you add any
  // new constraints to the schema
  const std::string sql = base::StringPrintf(
      "CREATE TABLE %s "
          "(id INTEGER PRIMARY KEY, "
          "creative_set_id LONGVARCHAR NOT NULL, "
          "type LONGVARCHAR NOT NULL, "
          "url_pattern LONGVARCHAR NOT NULL, "
          "observation_window INTEGER NOT NULL)",
      table_name);

  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::TruncateAdConversionsTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql =
      "DELETE FROM ad_conversions";

  sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

  return statement.Run();
}

bool BundleStateDatabase::InsertOrUpdateAdConversion(
    const ads::AdConversionInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql = base::StringPrintf(
      "INSERT OR REPLACE INTO ad_conversions "
          "(creative_set_id, "
          "type, "
          "url_pattern, "
          "observation_window) VALUES (%s)",
      CreateBindingParameterPlaceholders(4).c_str());

  sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

  statement.BindString(0, info.creative_set_id);
  statement.BindString(1, info.type);
  statement.BindString(2, info.url_pattern);
  statement.BindInt(3, info.observation_window);

  return statement.Run();
}

bool BundleStateDatabase::SaveBundleState(
    const ads::BundleState& bundle_state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  if (!GetDB().BeginTransaction()) {
    return false;
  }

  // We are completely replacing the database here so truncate all the tables
  if (!TruncateCategoriesTable() ||
      !TruncateCreativeAdNotificationCategoriesTable() ||
      !TruncateCreativeAdNotificationsTable() ||
      !TruncateCreativePublisherAdsCategoriesTable() ||
      !TruncateCreativePublisherAdsTable() ||
      !TruncateCreativePublisherAdsChannelsTable() ||
      !TruncateAdConversionsTable()) {
    GetDB().RollbackTransaction();
    return false;
  }

  for (const auto& creative_ad_notification :
      bundle_state.creative_ad_notifications) {
    const std::string category = creative_ad_notification.first;
    if (!InsertOrUpdateCategory(category)) {
      GetDB().RollbackTransaction();
      return false;
    }

    const ads::CreativeAdNotificationList ads = creative_ad_notification.second;
    for (const auto& ad : ads) {
      if (!InsertOrUpdateCreativeAdNotification(ad) ||
          !InsertOrUpdateCreativeAdNotificationCategory(ad, category)) {
        GetDB().RollbackTransaction();
        return false;
      }
    }
  }

  for (const auto& creative_publisher_ad :
      bundle_state.creative_publisher_ads) {
    const std::string category = creative_publisher_ad.first;
    if (!InsertOrUpdateCategory(category)) {
      GetDB().RollbackTransaction();
      return false;
    }

    const ads::CreativePublisherAdList ads = creative_publisher_ad.second;
    for (const auto& ad : ads) {
      if (!InsertOrUpdateCreativePublisherAd(ad) ||
          !InsertOrUpdateCreativePublisherAdCategory(ad, category) ||
          !InsertOrUpdateCreativePublisherAdChannel(ad)) {
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

bool BundleStateDatabase::GetCreativeAdNotifications(
    const std::vector<std::string>& categories,
    ads::CreativeAdNotificationList* ads) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(ads);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql = base::StringPrintf(
      "SELECT "
          "ai.creative_set_id, "
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
      "WHERE aic.category_name IN (%s) "
          "AND strftime('%%Y-%%m-%%d %%H:%%M', datetime('now','localtime')) "
              "BETWEEN ai.start_timestamp AND ai.end_timestamp",
      CreateBindingParameterPlaceholders(categories.size()).c_str());

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

bool BundleStateDatabase::GetCreativePublisherAds(
    const std::string& url,
    const std::vector<std::string>& categories,
    const std::vector<std::string>& sizes,
    ads::CreativePublisherAdList* ads) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(ads);

  GURL gurl = GURL(url);
  if (!gurl.is_valid()) {
    return true;
  }

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql = base::StringPrintf(
      "SELECT "
          "cpa.creative_instance_id, "
          "cpa.creative_set_id, "
          "cpa.campaign_id, "
          "cpa.start_at_timestamp, "
          "cpa.end_at_timestamp, "
          "cpa.daily_cap, "
          "cpa.advertiser_id, "
          "cpa.per_day, "
          "cpa.total_max, "
          "cpac.category, "
          "cpa.geo_target, "
          "cpa.size, "
          "cpa.creative_url, "
          "cpa.target_url, "
          "cpa.channel "
      "FROM creative_publisher_ads AS cpa "
          "INNER JOIN creative_publisher_ads_categories AS cpac "
              "ON cpac.creative_instance_id = cpa.creative_instance_id "
      "WHERE cpac.category IN (%s) "
          "AND cpa.size IN (%s) "
          "AND cpa.channel = ? "
          "AND strftime('%%Y-%%m-%%d %%H:%%M', datetime('now','localtime')) "
              "BETWEEN cpa.start_at_timestamp AND cpa.end_at_timestamp",
      CreateBindingParameterPlaceholders(categories.size()).c_str(),
      CreateBindingParameterPlaceholders(sizes.size()).c_str());

  sql::Statement statement(db_.GetUniqueStatement(sql.c_str()));

  int index = 0;

  for (const auto& category : categories) {
    statement.BindString(index, category.c_str());
    index++;
  }

  for (const auto& size : sizes) {
    statement.BindString(index, size.c_str());
    index++;
  }

  const std::string channel = GetPublisherAdsChannel(url);
  statement.BindString(index, channel);

  while (statement.Step()) {
    ads::CreativePublisherAdInfo info;
    info.creative_instance_id = statement.ColumnString(0);
    info.creative_set_id = statement.ColumnString(1);
    info.campaign_id = statement.ColumnString(2);
    info.start_at_timestamp = statement.ColumnString(3);
    info.end_at_timestamp = statement.ColumnString(4);
    info.daily_cap = statement.ColumnInt(5);
    info.advertiser_id = statement.ColumnString(6);
    info.per_day = statement.ColumnInt(7);
    info.total_max = statement.ColumnInt(8);
    info.category = statement.ColumnString(9);
    info.geo_targets.push_back(statement.ColumnString(10));
    info.size = statement.ColumnString(11);
    info.creative_url = statement.ColumnString(12);
    info.target_url = statement.ColumnString(13);
    info.channels.push_back(statement.ColumnString(14));
    ads->emplace_back(info);
  }

  return true;
}

bool BundleStateDatabase::GetCreativePublisherAdsToPreFetch(
    const std::vector<std::string>& creative_instance_ids,
    ads::CreativePublisherAdList* ads) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(ads);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql = base::StringPrintf(
      "SELECT DISTINCT "
          "cpa.creative_instance_id, "
          "cpa.creative_set_id, "
          "cpa.campaign_id, "
          "cpa.start_at_timestamp, "
          "cpa.end_at_timestamp, "
          "cpa.daily_cap, "
          "cpa.advertiser_id, "
          "cpa.per_day, "
          "cpa.total_max, "
          "cpa.geo_target, "
          "cpa.size, "
          "cpa.creative_url, "
          "cpa.target_url, "
          "cpa.channel "
      "FROM creative_publisher_ads AS cpa "
      "WHERE cpa.creative_instance_id NOT IN (%s) "
          "AND strftime('%%Y-%%m-%%d %%H:%%M', datetime('now','localtime')) "
              "BETWEEN cpa.start_at_timestamp AND cpa.end_at_timestamp "
      "ORDER BY Random() "
      "LIMIT 5",
      CreateBindingParameterPlaceholders(
          creative_instance_ids.size()).c_str());

  sql::Statement statement(db_.GetUniqueStatement(sql.c_str()));

  int index = 0;
  for (const auto& creative_instance_id : creative_instance_ids) {
    statement.BindString(index, creative_instance_id.c_str());
    index++;
  }

  while (statement.Step()) {
    const std::string creative_instance_id = statement.ColumnString(0);

    auto iter = std::find_if(ads->begin(), ads->end(),
        [&creative_instance_id](const ads::CreativePublisherAdInfo& info) {
            return info.creative_instance_id == creative_instance_id;
        });

    if (iter == ads->end()) {
      ads::CreativePublisherAdInfo info;
      info.creative_instance_id = creative_instance_id;
      info.creative_set_id = statement.ColumnString(1);
      info.campaign_id = statement.ColumnString(2);
      info.start_at_timestamp = statement.ColumnString(3);
      info.end_at_timestamp = statement.ColumnString(4);
      info.daily_cap = statement.ColumnInt(5);
      info.advertiser_id = statement.ColumnString(6);
      info.per_day = statement.ColumnInt(7);
      info.total_max = statement.ColumnInt(8);
      info.category = statement.ColumnString(9);
      info.geo_targets.push_back(statement.ColumnString(10));
      info.size = statement.ColumnString(11);
      info.creative_url = statement.ColumnString(12);
      info.target_url = statement.ColumnString(13);
      info.channels.push_back(statement.ColumnString(14));
      ads->emplace_back(info);

      continue;
    }

    iter->geo_targets.push_back(statement.ColumnString(10));
    iter->channels.push_back(statement.ColumnString(14));
  }

  return true;
}

bool BundleStateDatabase::GetExpiredCreativePublisherAds(
    const std::vector<std::string>& creative_instance_ids,
    ads::CreativePublisherAdList* ads) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(ads);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql = base::StringPrintf(
      "SELECT "
          "cpa.creative_instance_id, "
          "cpa.creative_set_id, "
          "cpa.campaign_id, "
          "cpa.start_at_timestamp, "
          "cpa.end_at_timestamp, "
          "cpa.daily_cap, "
          "cpa.advertiser_id, "
          "cpa.per_day, "
          "cpa.total_max, "
          "cpa.geo_target, "
          "cpa.size, "
          "cpa.creative_url, "
          "cpa.target_url, "
          "cpa.channel "
      "FROM creative_publisher_ads AS cpa "
      "WHERE cpa.creative_instance_id IN (%s) "
          "AND strftime('%%Y-%%m-%%d %%H:%%M', datetime('now','localtime')) "
              "NOT BETWEEN cpa.start_at_timestamp AND cpa.end_at_timestamp",
      CreateBindingParameterPlaceholders(
          creative_instance_ids.size()).c_str());

  sql::Statement statement(db_.GetUniqueStatement(sql.c_str()));

  int index = 0;
  for (const auto& creative_instance_id : creative_instance_ids) {
    statement.BindString(index, creative_instance_id.c_str());
    index++;
  }

  while (statement.Step()) {
    const std::string creative_instance_id = statement.ColumnString(0);

    auto iter = std::find_if(ads->begin(), ads->end(),
        [&creative_instance_id](const ads::CreativePublisherAdInfo& info) {
            return info.creative_instance_id == creative_instance_id;
        });

    if (iter == ads->end()) {
      ads::CreativePublisherAdInfo info;
      info.creative_instance_id = creative_instance_id;
      info.creative_set_id = statement.ColumnString(1);
      info.campaign_id = statement.ColumnString(2);
      info.start_at_timestamp = statement.ColumnString(3);
      info.end_at_timestamp = statement.ColumnString(4);
      info.daily_cap = statement.ColumnInt(5);
      info.advertiser_id = statement.ColumnString(6);
      info.per_day = statement.ColumnInt(7);
      info.total_max = statement.ColumnInt(8);
      info.category = statement.ColumnString(9);
      info.geo_targets.push_back(statement.ColumnString(10));
      info.size = statement.ColumnString(11);
      info.creative_url = statement.ColumnString(12);
      info.target_url = statement.ColumnString(13);
      info.channels.push_back(statement.ColumnString(14));
      ads->emplace_back(info);

      continue;
    }

    iter->geo_targets.push_back(statement.ColumnString(10));
    iter->channels.push_back(statement.ColumnString(14));
  }

  return true;
}

bool BundleStateDatabase::SiteSupportsPublisherAds(
    const std::string& url,
    bool* is_supported) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(is_supported);
  *is_supported = false;

  GURL gurl = GURL(url);
  if (!gurl.is_valid()) {
    return true;
  }

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string channel = GetPublisherAdsChannel(url);

  const std::string sql = base::StringPrintf(
      "SELECT EXISTS "
          "(SELECT 1 FROM creative_publisher_ads_channels "
              "WHERE channel = %s)",
      CreateBindingParameterPlaceholders(1).c_str());

  sql::Statement statement(GetDB().GetUniqueStatement(sql.c_str()));

  statement.BindString(0, channel);

  if (statement.Step()) {
    *is_supported = statement.ColumnBool(0);
  }

  return true;
}

bool BundleStateDatabase::GetAdConversions(
    const std::string& url,
    ads::AdConversionList* ad_conversions) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(ad_conversions);

  const bool is_initialized = Init();
  DCHECK(is_initialized);

  const std::string sql =
      "SELECT "
          "ac.creative_set_id, "
          "ac.type, ac.url_pattern, "
          "ac.observation_window "
      "FROM ad_conversions AS ac";

  sql::Statement statement(db_.GetUniqueStatement(sql.c_str()));

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

std::string BundleStateDatabase::GetPublisherAdsChannel(
    const std::string& url) {
  const std::string tld_plus_1 = GetDomainAndRegistry(GURL(url),
      net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);
  return tld_plus_1;
}

std::string BundleStateDatabase::CreateBindingParameterPlaceholders(
    const size_t count) {
  std::string placeholders;

  for (size_t i = 0; i < count; i++) {
    if (i != 0) {
      placeholders += ", ";
    }

    placeholders += "?";
  }

  return placeholders;
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
      "Can not have a transaction when vacuuming";
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

  DCHECK(statement);

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

  if (meta_table_.GetCompatibleVersionNumber() > GetCurrentVersion()) {
    LOG(WARNING) << "Bundle state database cannot be downgraded";
    return false;
  }

  const int source_version = meta_table_.GetVersionNumber();
  const int dest_version = GetCurrentVersion();

  bool success = false;
  for (int i = source_version; i < dest_version; i++) {
    switch (i) {
      case 1: {
        success = MigrateV1toV2();
        break;
      }

      case 2: {
        success = MigrateV2toV3();
        break;
      }

      case 3: {
        success = MigrateV3toV4();
        break;
      }

      case 4: {
        success = MigrateV4toV5();
        break;
      }

      default: {
        NOTREACHED();
        break;
      }
    }

    if (!success) {
      LOG(ERROR) << "Cannot migrate database from v" << source_version << " to "
          "v" << dest_version;

      return false;
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
  const std::string campaign_id_sql =
      "ALTER TABLE ad_info "
          "ADD campaign_id LONGVARCHAR";
  if (!GetDB().Execute(campaign_id_sql.c_str())) {
    return false;
  }

  const std::string daily_cap_sql =
      "ALTER TABLE ad_info "
          "ADD daily_cap INTEGER DEFAULT 0 NOT NULL";
  if (!GetDB().Execute(daily_cap_sql.c_str())) {
    return false;
  }

  const std::string per_day_sql =
      "ALTER TABLE ad_info "
          "ADD per_day INTEGER DEFAULT 0 NOT NULL";
  if (!GetDB().Execute(per_day_sql.c_str())) {
    return false;
  }

  const std::string total_max_sql =
      "ALTER TABLE ad_info "
          "ADD total_max INTEGER DEFAULT 0 NOT NULL";
  if (!GetDB().Execute(total_max_sql.c_str())) {
    return false;
  }

  return true;
}

bool BundleStateDatabase::MigrateV2toV3() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char table_name[] = "ad_conversions";
  if (GetDB().DoesTableExist(table_name)) {
    return true;
  }

  const std::string sql = base::StringPrintf(
      "CREATE TABLE %s "
          "(id INTEGER PRIMARY KEY, "
          "creative_set_id LONGVARCHAR NOT NULL, "
          "type LONGVARCHAR NOT NULL, "
          "url_pattern LONGVARCHAR NOT NULL, "
          "observation_window INTEGER NOT NULL)",
      table_name);

  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::MigrateV3toV4() {
  const std::string sql =
      "ALTER TABLE ad_info "
      "ADD advertiser_id LONGVARCHAR";

  return GetDB().Execute(sql.c_str());
}

bool BundleStateDatabase::MigrateV4toV5() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char creative_publisher_ads_table_name[] = "creative_publisher_ads";
  if (!GetDB().DoesTableExist(creative_publisher_ads_table_name)) {
    const std::string sql = base::StringPrintf(
        "CREATE TABLE %s "
            "(creative_instance_id LONGVARCHAR, "
            "creative_set_id LONGVARCHAR, "
            "campaign_id LONGVARCHAR, "
            "start_at_timestamp DATETIME, "
            "end_at_timestamp DATETIME, "
            "daily_cap INTEGER DEFAULT 0 NOT NULL, "
            "advertiser_id LONGVARCHAR, "
            "per_day INTEGER DEFAULT 0 NOT NULL, "
            "total_max INTEGER DEFAULT 0 NOT NULL, "
            "geo_target VARCHAR, "
            "size TEXT, "
            "creative_url LONGVARCHAR, "
            "target_url LONGVARCHAR, "
            "channel VARCHAR, "
            "PRIMARY KEY (creative_instance_id, geo_target, channel))",
        creative_publisher_ads_table_name);

    if (!GetDB().Execute(sql.c_str())) {
      return false;
    }
  }

  const char creative_publisher_ads_channels_table_name[] =
      "creative_publisher_ads_channels";
  if (!GetDB().DoesTableExist(creative_publisher_ads_channels_table_name)) {
    const std::string sql = base::StringPrintf(
        "CREATE TABLE %s "
            "(channel LONGVARCHAR PRIMARY KEY)",
        creative_publisher_ads_channels_table_name);

    if (!GetDB().Execute(sql.c_str())) {
      return false;
    }
  }

  return true;
}

}  // namespace brave_ads
