/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"

#include <functional>
#include <set>
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
      geo_targets_database_table_(std::make_unique<GeoTargets>(ads_)),
      categories_database_table_(std::make_unique<Categories>(ads_)) {
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

  // TODO(https://github.com/brave/brave-browser/issues/3661) instead of
  // rebuilding the catalog database each time
  DeleteAllTables(transaction.get());

  const std::vector<CreativeAdNotificationList> batches =
      SplitVector(creative_ad_notifications, batch_size_);

  for (const auto& batch : batches) {
    InsertOrUpdate(transaction.get(), batch);
    geo_targets_database_table_->InsertOrUpdate(transaction.get(), batch);
    categories_database_table_->InsertOrUpdate(transaction.get(), batch);
  }

  ads_->get_ads_client()->RunDBTransaction(std::move(transaction),
      std::bind(&OnResultCallback, _1, callback));
}

void CreativeAdNotifications::GetCreativeAdNotifications(
    const classification::CategoryList& categories,
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
          "can.start_at_timestamp, "
          "can.end_at_timestamp, "
          "can.daily_cap, "
          "can.advertiser_id, "
          "can.priority, "
          "can.conversion, "
          "can.per_day, "
          "can.total_max, "
          "c.category, "
          "gt.geo_target, "
          "can.target_url, "
          "can.title, "
          "can.body "
      "FROM %s AS can "
          "INNER JOIN categories AS c "
              "ON c.creative_instance_id = can.creative_instance_id "
          "INNER JOIN geo_targets AS gt "
              "ON gt.creative_instance_id = can.creative_instance_id "
      "WHERE c.category IN %s "
          "AND %s BETWEEN can.start_at_timestamp AND can.end_at_timestamp",
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
    DBCommand::RecordBindingType::STRING_TYPE   // body
  };

  DBTransactionPtr transaction = DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ads_->get_ads_client()->RunDBTransaction(std::move(transaction),
      std::bind(&CreativeAdNotifications::OnGetCreativeAdNotifications, this,
          _1, categories, callback));
}

void CreativeAdNotifications::GetAllCreativeAdNotifications(
    GetCreativeAdNotificationsCallback callback) {
  const std::string query = base::StringPrintf(
      "SELECT "
          "can.creative_instance_id, "
          "can.creative_set_id, "
          "can.campaign_id, "
          "can.start_at_timestamp, "
          "can.end_at_timestamp, "
          "can.daily_cap, "
          "can.advertiser_id, "
          "can.priority, "
          "can.conversion, "
          "can.per_day, "
          "can.total_max, "
          "c.category, "
          "gt.geo_target, "
          "can.target_url, "
          "can.title, "
          "can.body "
      "FROM %s AS can "
          "INNER JOIN categories AS c "
              "ON c.creative_instance_id = can.creative_instance_id "
          "INNER JOIN geo_targets AS gt "
              "ON gt.creative_instance_id = can.creative_instance_id "
      "WHERE %s BETWEEN can.start_at_timestamp AND can.end_at_timestamp",
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
    DBCommand::RecordBindingType::STRING_TYPE   // body
  };

  DBTransactionPtr transaction = DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ads_->get_ads_client()->RunDBTransaction(std::move(transaction),
      std::bind(&CreativeAdNotifications::OnGetAllCreativeAdNotifications,
          this, _1, callback));
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
    BindInt64(command, index++, creative_ad_notification.start_at_timestamp);
    BindInt64(command, index++, creative_ad_notification.end_at_timestamp);
    BindInt64(command, index++, creative_ad_notification.daily_cap);
    BindString(command, index++, creative_ad_notification.advertiser_id);
    BindInt64(command, index++, creative_ad_notification.priority);
    BindBool(command, index++, creative_ad_notification.conversion);
    BindInt64(command, index++, creative_ad_notification.per_day);
    BindInt64(command, index++, creative_ad_notification.total_max);
    BindString(command, index++, creative_ad_notification.target_url);
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
          "start_at_timestamp, "
          "end_at_timestamp, "
          "daily_cap, "
          "advertiser_id, "
          "priority, "
          "conversion, "
          "per_day, "
          "total_max, "
          "target_url, "
          "title, "
          "body) VALUES %s",
      get_table_name().c_str(),
      BuildBindingParameterPlaceholders(14, count).c_str());
}

void CreativeAdNotifications::OnGetCreativeAdNotifications(
    DBCommandResponsePtr response,
    const classification::CategoryList& categories,
    GetCreativeAdNotificationsCallback callback) {
  if (!response || response->status != DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get creative ad notifications");
    callback(Result::FAILED, categories, {});
    return;
  }

  CreativeAdNotificationList creative_ad_notifications;

  for (auto const& record : response->result->get_records()) {
    const CreativeAdNotificationInfo info =
        GetCreativeAdNotificationFromRecord(record.get());

    creative_ad_notifications.emplace_back(info);
  }

  callback(Result::SUCCESS, categories, creative_ad_notifications);
}

void CreativeAdNotifications::OnGetAllCreativeAdNotifications(
    DBCommandResponsePtr response,
    GetCreativeAdNotificationsCallback callback) {
  if (!response || response->status != DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get all creative ad notifications");
    callback(Result::FAILED, {}, {});
    return;
  }

  CreativeAdNotificationList creative_ad_notifications;

  std::set<std::string> categories;

  for (auto const& record : response->result->get_records()) {
    const CreativeAdNotificationInfo info =
        GetCreativeAdNotificationFromRecord(record.get());

    creative_ad_notifications.emplace_back(info);

    categories.insert(info.category);
  }

  classification::CategoryList normalized_categories;
  for (const auto& category : categories) {
    normalized_categories.push_back(category);
  }

  callback(Result::SUCCESS, normalized_categories, creative_ad_notifications);
}

CreativeAdNotificationInfo
CreativeAdNotifications::GetCreativeAdNotificationFromRecord(
    DBRecord* record) const {
  CreativeAdNotificationInfo info;

  info.creative_instance_id = ColumnString(record, 0);
  info.creative_set_id = ColumnString(record, 1);
  info.campaign_id = ColumnString(record, 2);
  info.start_at_timestamp = ColumnInt64(record, 3);
  info.end_at_timestamp = ColumnInt64(record, 4);
  info.daily_cap = ColumnInt(record, 5);
  info.advertiser_id = ColumnString(record, 6);
  info.priority = ColumnInt(record, 7);
  info.conversion = ColumnBool(record, 8);
  info.per_day = ColumnInt(record, 9);
  info.total_max = ColumnInt(record, 10);
  info.category = ColumnString(record, 11);
  info.geo_targets.push_back(ColumnString(record, 12));
  info.target_url = ColumnString(record, 13);
  info.title = ColumnString(record, 14);
  info.body = ColumnString(record, 15);

  return info;
}

void CreativeAdNotifications::DeleteAllTables(
    DBTransaction* transaction) const {
  Delete(transaction, get_table_name());
  Delete(transaction, geo_targets_database_table_->get_table_name());
  Delete(transaction, categories_database_table_->get_table_name());
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

  Drop(transaction, get_table_name());

  CreateTableV1(transaction);
}

}  // namespace table
}  // namespace database
}  // namespace ads
