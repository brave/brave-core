/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"

#include <algorithm>
#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/container_util.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_util.h"

namespace ads {
namespace database {
namespace table {

using std::placeholders::_1;

namespace {

const char kTableName[] = "creative_ad_notifications";

const int kDefaultBatchSize = 50;

}  // namespace

CreativeAdNotifications::CreativeAdNotifications(
    AdsImpl* ads)
    : batch_size_(kDefaultBatchSize),
      ads_(ads),
      campaigns_database_table_(std::make_unique<Campaigns>(ads_)),
      categories_database_table_(std::make_unique<Categories>(ads_)),
      creative_ads_database_table_(std::make_unique<CreativeAds>(ads_)),
      dayparts_database_table_(std::make_unique<Dayparts>(ads_)),
      geo_targets_database_table_(std::make_unique<GeoTargets>(ads_)) {
  DCHECK(ads_);
}

CreativeAdNotifications::~CreativeAdNotifications() = default;

void CreativeAdNotifications::Save(
    const CreativeAdNotificationList& creative_ad_notifications,
    ResultCallback callback) {
  if (creative_ad_notifications.empty()) {
    callback(Result::SUCCESS);
    return;
  }

  DBTransactionPtr transaction = DBTransaction::New();

  const std::vector<CreativeAdNotificationList> batches =
      SplitVector(creative_ad_notifications, batch_size_);

  for (const auto& batch : batches) {
    InsertOrUpdate(transaction.get(), batch);

    std::vector<CreativeAdInfo> creative_ads(batch.begin(), batch.end());
    campaigns_database_table_->InsertOrUpdate(
        transaction.get(), creative_ads);
    categories_database_table_->InsertOrUpdate(
        transaction.get(), creative_ads);
    creative_ads_database_table_->InsertOrUpdate(
        transaction.get(), creative_ads);
    dayparts_database_table_->InsertOrUpdate(transaction.get(), creative_ads);
    geo_targets_database_table_->InsertOrUpdate(
        transaction.get(), creative_ads);
  }

  ads_->get_ads_client()->RunDBTransaction(std::move(transaction),
      std::bind(&OnResultCallback, _1, callback));
}

void CreativeAdNotifications::Delete(
    ResultCallback callback) {
  DBTransactionPtr transaction = DBTransaction::New();

  util::Delete(transaction.get(), get_table_name());

  ads_->get_ads_client()->RunDBTransaction(std::move(transaction),
      std::bind(&OnResultCallback, _1, callback));
}

void CreativeAdNotifications::GetForCategories(
    const CategoryList& categories,
    GetCreativeAdNotificationsCallback callback) {
  if (categories.empty()) {
    callback(Result::SUCCESS, categories, {});
    return;
  }

  const std::string query = base::StringPrintf(
      "SELECT "
          "can.creative_instance_id, "
          "can.creative_set_id, "
          "can.campaign_id, "
          "cam.start_at_timestamp, "
          "cam.end_at_timestamp, "
          "cam.daily_cap, "
          "cam.advertiser_id, "
          "cam.priority, "
          "ca.conversion, "
          "ca.per_day, "
          "ca.total_max, "
          "c.category, "
          "gt.geo_target, "
          "ca.target_url, "
          "can.title, "
          "can.body, "
          "cam.ptr, "
          "dp.dow, "
          "dp.start_minute, "
          "dp.end_minute "
      "FROM %s AS can "
          "INNER JOIN campaigns AS cam "
              "ON cam.campaign_id = can.campaign_id "
          "INNER JOIN categories AS c "
              "ON c.creative_set_id = can.creative_set_id "
          "INNER JOIN creative_ads AS ca "
              "ON ca.creative_set_id = can.creative_set_id "
          "INNER JOIN geo_targets AS gt "
              "ON gt.campaign_id = can.campaign_id "
          "INNER JOIN dayparts AS dp "
              "ON dp.campaign_id = can.campaign_id "
      "WHERE c.category IN %s "
          "AND %s BETWEEN cam.start_at_timestamp AND cam.end_at_timestamp",
      get_table_name().c_str(),
      BuildBindingParameterPlaceholder(categories.size()).c_str(),
      NowAsString().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::READ;
  command->command = query;

  int index = 0;
  for (const auto& category : categories) {
    BindString(command.get(), index, base::ToLowerASCII(category));
    index++;
  }

  command->record_bindings = {
    DBCommand::RecordBindingType::STRING_TYPE,  // creative_instance_id
    DBCommand::RecordBindingType::STRING_TYPE,  // creative_set_id
    DBCommand::RecordBindingType::STRING_TYPE,  // campaign_id
    DBCommand::RecordBindingType::INT64_TYPE,   // start_at_timestamp
    DBCommand::RecordBindingType::INT64_TYPE,   // end_at_timestamp
    DBCommand::RecordBindingType::INT_TYPE,     // daily_cap
    DBCommand::RecordBindingType::STRING_TYPE,  // advertiser_id
    DBCommand::RecordBindingType::INT_TYPE,     // priority
    DBCommand::RecordBindingType::BOOL_TYPE,    // conversion
    DBCommand::RecordBindingType::INT_TYPE,     // per_day
    DBCommand::RecordBindingType::INT_TYPE,     // total_max
    DBCommand::RecordBindingType::STRING_TYPE,  // category
    DBCommand::RecordBindingType::STRING_TYPE,  // geo_target
    DBCommand::RecordBindingType::STRING_TYPE,  // target_url
    DBCommand::RecordBindingType::STRING_TYPE,  // title
    DBCommand::RecordBindingType::STRING_TYPE,  // body
    DBCommand::RecordBindingType::DOUBLE_TYPE,  // ptr
    DBCommand::RecordBindingType::STRING_TYPE,  // dayparts->dow
    DBCommand::RecordBindingType::INT_TYPE,     // dayparts->start_minute
    DBCommand::RecordBindingType::INT_TYPE      // dayparts->end_minute
  };

  DBTransactionPtr transaction = DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ads_->get_ads_client()->RunDBTransaction(std::move(transaction),
      std::bind(&CreativeAdNotifications::OnGetForCategories, this, _1,
          categories, callback));
}

void CreativeAdNotifications::GetAll(
    GetCreativeAdNotificationsCallback callback) {
  const std::string query = base::StringPrintf(
      "SELECT "
          "can.creative_instance_id, "
          "can.creative_set_id, "
          "can.campaign_id, "
          "cam.start_at_timestamp, "
          "cam.end_at_timestamp, "
          "cam.daily_cap, "
          "cam.advertiser_id, "
          "cam.priority, "
          "ca.conversion, "
          "ca.per_day, "
          "ca.total_max, "
          "c.category, "
          "gt.geo_target, "
          "ca.target_url, "
          "can.title, "
          "can.body, "
          "cam.ptr, "
          "dp.dow, "
          "dp.start_minute, "
          "dp.end_minute "
      "FROM %s AS can "
          "INNER JOIN campaigns AS cam "
              "ON cam.campaign_id = can.campaign_id "
          "INNER JOIN categories AS c "
              "ON c.creative_set_id = can.creative_set_id "
          "INNER JOIN creative_ads AS ca "
              "ON ca.creative_set_id = can.creative_set_id "
          "INNER JOIN geo_targets AS gt "
              "ON gt.campaign_id = can.campaign_id "
          "INNER JOIN dayparts AS dp "
              "ON dp.campaign_id = can.campaign_id "
      "WHERE %s BETWEEN cam.start_at_timestamp AND cam.end_at_timestamp",
      get_table_name().c_str(),
      NowAsString().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
    DBCommand::RecordBindingType::STRING_TYPE,  // creative_instance_id
    DBCommand::RecordBindingType::STRING_TYPE,  // creative_set_id
    DBCommand::RecordBindingType::STRING_TYPE,  // campaign_id
    DBCommand::RecordBindingType::INT64_TYPE,   // start_at_timestamp
    DBCommand::RecordBindingType::INT64_TYPE,   // end_at_timestamp
    DBCommand::RecordBindingType::INT_TYPE,     // daily_cap
    DBCommand::RecordBindingType::STRING_TYPE,  // advertiser_id
    DBCommand::RecordBindingType::INT_TYPE,     // priority
    DBCommand::RecordBindingType::BOOL_TYPE,    // conversion
    DBCommand::RecordBindingType::INT_TYPE,     // per_day
    DBCommand::RecordBindingType::INT_TYPE,     // total_max
    DBCommand::RecordBindingType::STRING_TYPE,  // category
    DBCommand::RecordBindingType::STRING_TYPE,  // geo_target
    DBCommand::RecordBindingType::STRING_TYPE,  // target_url
    DBCommand::RecordBindingType::STRING_TYPE,  // title
    DBCommand::RecordBindingType::STRING_TYPE,  // body
    DBCommand::RecordBindingType::DOUBLE_TYPE,  // ptr
    DBCommand::RecordBindingType::STRING_TYPE,  // dayparts->dow
    DBCommand::RecordBindingType::INT_TYPE,     // dayparts->start_minute
    DBCommand::RecordBindingType::INT_TYPE      // dayparts->end_minute
  };

  DBTransactionPtr transaction = DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ads_->get_ads_client()->RunDBTransaction(std::move(transaction),
      std::bind(&CreativeAdNotifications::OnGetAll, this, _1, callback));
}

void CreativeAdNotifications::set_batch_size(
    const int batch_size) {
  DCHECK_GT(batch_size, 0);

  batch_size_ = batch_size;
}

std::string CreativeAdNotifications::get_table_name() const {
  return kTableName;
}

void CreativeAdNotifications::Migrate(
    DBTransaction* transaction,
    const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 1: {
      MigrateToV1(transaction);
      break;
    }

    case 2: {
      MigrateToV2(transaction);
      break;
    }

    case 3: {
      MigrateToV3(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeAdNotifications::InsertOrUpdate(
    DBTransaction* transaction,
    const CreativeAdNotificationList& creative_ad_notifications) {
  DCHECK(transaction);

  if (creative_ad_notifications.empty()) {
    return;
  }

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(),
      creative_ad_notifications);

  transaction->commands.push_back(std::move(command));
}

int CreativeAdNotifications::BindParameters(
    DBCommand* command,
    const CreativeAdNotificationList& creative_ad_notifications) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& creative_ad_notification : creative_ad_notifications) {
    BindString(command, index++, creative_ad_notification.creative_instance_id);
    BindString(command, index++, creative_ad_notification.creative_set_id);
    BindString(command, index++, creative_ad_notification.campaign_id);
    BindString(command, index++, creative_ad_notification.title);
    BindString(command, index++, creative_ad_notification.body);

    count++;
  }

  return count;
}

std::string CreativeAdNotifications::BuildInsertOrUpdateQuery(
    DBCommand* command,
    const CreativeAdNotificationList& creative_ad_notifications) {
  const int count = BindParameters(command, creative_ad_notifications);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
          "(creative_instance_id, "
          "creative_set_id, "
          "campaign_id, "
          "title, "
          "body) VALUES %s",
      get_table_name().c_str(),
      BuildBindingParameterPlaceholders(5, count).c_str());
}

void CreativeAdNotifications::OnGetForCategories(
    DBCommandResponsePtr response,
    const CategoryList& categories,
    GetCreativeAdNotificationsCallback callback) {
  if (!response || response->status != DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get creative ad notifications");
    callback(Result::FAILED, categories, {});
    return;
  }

  CreativeAdNotificationList creative_ad_notifications;

  for (const auto& record : response->result->get_records()) {
    const CreativeAdNotificationInfo creative_ad_notification =
        GetFromRecord(record.get());

    creative_ad_notifications.push_back(creative_ad_notification);
  }

  callback(Result::SUCCESS, categories, creative_ad_notifications);
}

void CreativeAdNotifications::OnGetAll(
    DBCommandResponsePtr response,
    GetCreativeAdNotificationsCallback callback) {
  if (!response || response->status != DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get all creative ad notifications");
    callback(Result::FAILED, {}, {});
    return;
  }

  CreativeAdNotificationList creative_ad_notifications;

  CategoryList categories;

  for (const auto& record : response->result->get_records()) {
    const CreativeAdNotificationInfo creative_ad_notification =
        GetFromRecord(record.get());

    creative_ad_notifications.push_back(creative_ad_notification);

    categories.push_back(creative_ad_notification.category);
  }

  std::sort(categories.begin(), categories.end());
  const auto iter = std::unique(categories.begin(), categories.end());
  categories.erase(iter, categories.end());

  callback(Result::SUCCESS, categories, creative_ad_notifications);
}

CreativeAdNotificationInfo CreativeAdNotifications::GetFromRecord(
    DBRecord* record) const {
  CreativeAdNotificationInfo creative_ad_notification;

  creative_ad_notification.creative_instance_id = ColumnString(record, 0);
  creative_ad_notification.creative_set_id = ColumnString(record, 1);
  creative_ad_notification.campaign_id = ColumnString(record, 2);
  creative_ad_notification.start_at_timestamp = ColumnInt64(record, 3);
  creative_ad_notification.end_at_timestamp = ColumnInt64(record, 4);
  creative_ad_notification.daily_cap = ColumnInt(record, 5);
  creative_ad_notification.advertiser_id = ColumnString(record, 6);
  creative_ad_notification.priority = ColumnInt(record, 7);
  creative_ad_notification.conversion = ColumnBool(record, 8);
  creative_ad_notification.per_day = ColumnInt(record, 9);
  creative_ad_notification.total_max = ColumnInt(record, 10);
  creative_ad_notification.category = ColumnString(record, 11);
  creative_ad_notification.geo_targets.push_back(ColumnString(record, 12));
  creative_ad_notification.target_url = ColumnString(record, 13);
  creative_ad_notification.title = ColumnString(record, 14);
  creative_ad_notification.body = ColumnString(record, 15);
  creative_ad_notification.ptr = ColumnDouble(record, 16);

  CreativeDaypartInfo daypart;
  daypart.dow = ColumnString(record, 17);
  daypart.start_minute = ColumnInt(record, 18);
  daypart.end_minute = ColumnInt(record, 19);
  creative_ad_notification.dayparts.push_back(daypart);

  return creative_ad_notification;
}

void CreativeAdNotifications::CreateTableV1(
    DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
          "(creative_instance_id TEXT NOT NULL, "
          "creative_set_id TEXT NOT NULL, "
          "campaign_id TEXT NOT NULL, "
          "start_at_timestamp TIMESTAMP NOT NULL, "
          "end_at_timestamp TIMESTAMP NOT NULL, "
          "daily_cap INTEGER DEFAULT 0 NOT NULL, "
          "advertiser_id LONGVARCHAR, "
          "priority INTEGER NOT NULL DEFAULT 0, "
          "conversion INTEGER NOT NULL DEFAULT 0, "
          "per_day INTEGER NOT NULL DEFAULT 0, "
          "total_max INTEGER NOT NULL DEFAULT 0, "
          "target_url TEXT NOT NULL, "
          "title TEXT NOT NULL, "
          "body TEXT NOT NULL, "
          "PRIMARY KEY(creative_instance_id))",
      get_table_name().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void CreativeAdNotifications::MigrateToV1(
    DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, get_table_name());

  CreateTableV1(transaction);
}

void CreativeAdNotifications::MigrateToV2(
    DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "ALTER TABLE %s "
          "ADD ptr DOUBLE NOT NULL DEFAULT 1",
      get_table_name().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void CreativeAdNotifications::CreateTableV3(
    DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
          "(creative_instance_id TEXT NOT NULL PRIMARY KEY UNIQUE "
              "ON CONFLICT REPLACE, "
          "creative_set_id TEXT NOT NULL, "
          "campaign_id TEXT NOT NULL, "
          "title TEXT NOT NULL, "
          "body TEXT NOT NULL)",
      get_table_name().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void CreativeAdNotifications::MigrateToV3(
    DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, get_table_name());

  CreateTableV3(transaction);
}

}  // namespace table
}  // namespace database
}  // namespace ads
