/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/creative_inline_content_ads_database_table.h"

#include <algorithm>
#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/container_util.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_formatting_util.h"

namespace ads {
namespace database {
namespace table {

namespace {

const char kTableName[] = "creative_inline_content_ads";

const int kDefaultBatchSize = 50;

}  // namespace

CreativeInlineContentAds::CreativeInlineContentAds()
    : batch_size_(kDefaultBatchSize),
      campaigns_database_table_(std::make_unique<Campaigns>()),
      creative_ads_database_table_(std::make_unique<CreativeAds>()),
      dayparts_database_table_(std::make_unique<Dayparts>()),
      geo_targets_database_table_(std::make_unique<GeoTargets>()),
      segments_database_table_(std::make_unique<Segments>()) {}

CreativeInlineContentAds::~CreativeInlineContentAds() = default;

void CreativeInlineContentAds::Save(
    const CreativeInlineContentAdList& creative_inline_content_ads,
    ResultCallback callback) {
  if (creative_inline_content_ads.empty()) {
    callback(Result::SUCCESS);
    return;
  }

  DBTransactionPtr transaction = DBTransaction::New();

  const std::vector<CreativeInlineContentAdList> batches =
      SplitVector(creative_inline_content_ads, batch_size_);

  for (const auto& batch : batches) {
    InsertOrUpdate(transaction.get(), batch);

    std::vector<CreativeAdInfo> creative_ads(batch.begin(), batch.end());
    campaigns_database_table_->InsertOrUpdate(transaction.get(), creative_ads);
    creative_ads_database_table_->InsertOrUpdate(transaction.get(),
                                                 creative_ads);
    dayparts_database_table_->InsertOrUpdate(transaction.get(), creative_ads);
    geo_targets_database_table_->InsertOrUpdate(transaction.get(),
                                                creative_ads);
    segments_database_table_->InsertOrUpdate(transaction.get(), creative_ads);
  }

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void CreativeInlineContentAds::Delete(ResultCallback callback) {
  DBTransactionPtr transaction = DBTransaction::New();

  util::Delete(transaction.get(), get_table_name());

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void CreativeInlineContentAds::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativeInlineContentAdCallback callback) {
  CreativeInlineContentAdInfo creative_inline_content_ad;

  if (creative_instance_id.empty()) {
    callback(Result::FAILED, creative_instance_id, creative_inline_content_ad);
    return;
  }

  const std::string query = base::StringPrintf(
      "SELECT "
      "cbna.creative_instance_id, "
      "cbna.creative_set_id, "
      "cbna.campaign_id, "
      "cam.start_at_timestamp, "
      "cam.end_at_timestamp, "
      "cam.daily_cap, "
      "cam.advertiser_id, "
      "cam.priority, "
      "ca.conversion, "
      "ca.per_day, "
      "ca.per_week, "
      "ca.per_month, "
      "ca.total_max, "
      "ca.split_test_group, "
      "s.segment, "
      "gt.geo_target, "
      "ca.target_url, "
      "cbna.title, "
      "cbna.description, "
      "cbna.image_url, "
      "cbna.dimensions, "
      "cbna.cta_text, "
      "cam.ptr, "
      "dp.dow, "
      "dp.start_minute, "
      "dp.end_minute "
      "FROM %s AS cbna "
      "INNER JOIN campaigns AS cam "
      "ON cam.campaign_id = cbna.campaign_id "
      "INNER JOIN segments AS s "
      "ON s.creative_set_id = cbna.creative_set_id "
      "INNER JOIN creative_ads AS ca "
      "ON ca.creative_instance_id = cbna.creative_instance_id "
      "INNER JOIN geo_targets AS gt "
      "ON gt.campaign_id = cbna.campaign_id "
      "INNER JOIN dayparts AS dp "
      "ON dp.campaign_id = cbna.campaign_id "
      "WHERE cbna.creative_instance_id = '%s'",
      get_table_name().c_str(), creative_instance_id.c_str());

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
      DBCommand::RecordBindingType::INT_TYPE,     // per_week
      DBCommand::RecordBindingType::INT_TYPE,     // per_month
      DBCommand::RecordBindingType::INT_TYPE,     // total_max
      DBCommand::RecordBindingType::STRING_TYPE,  // split_test_group
      DBCommand::RecordBindingType::STRING_TYPE,  // segment
      DBCommand::RecordBindingType::STRING_TYPE,  // geo_target
      DBCommand::RecordBindingType::STRING_TYPE,  // target_url
      DBCommand::RecordBindingType::STRING_TYPE,  // title
      DBCommand::RecordBindingType::STRING_TYPE,  // description
      DBCommand::RecordBindingType::STRING_TYPE,  // image_url
      DBCommand::RecordBindingType::STRING_TYPE,  // dimensions
      DBCommand::RecordBindingType::STRING_TYPE,  // cta_text
      DBCommand::RecordBindingType::DOUBLE_TYPE,  // ptr
      DBCommand::RecordBindingType::STRING_TYPE,  // dayparts->dow
      DBCommand::RecordBindingType::INT_TYPE,     // dayparts->start_minute
      DBCommand::RecordBindingType::INT_TYPE      // dayparts->end_minute
  };

  DBTransactionPtr transaction = DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&CreativeInlineContentAds::OnGetForCreativeInstanceId, this,
                std::placeholders::_1, creative_instance_id, callback));
}

void CreativeInlineContentAds::GetForSegments(
    const SegmentList& segments,
    const std::string& dimensions,
    GetCreativeInlineContentAdsCallback callback) {
  if (segments.empty() || dimensions.empty()) {
    callback(Result::SUCCESS, segments, {});
    return;
  }

  const std::string query = base::StringPrintf(
      "SELECT "
      "cbna.creative_instance_id, "
      "cbna.creative_set_id, "
      "cbna.campaign_id, "
      "cam.start_at_timestamp, "
      "cam.end_at_timestamp, "
      "cam.daily_cap, "
      "cam.advertiser_id, "
      "cam.priority, "
      "ca.conversion, "
      "ca.per_day, "
      "ca.per_week, "
      "ca.per_month, "
      "ca.total_max, "
      "ca.split_test_group, "
      "s.segment, "
      "gt.geo_target, "
      "ca.target_url, "
      "cbna.title, "
      "cbna.description, "
      "cbna.image_url, "
      "cbna.dimensions, "
      "cbna.cta_text, "
      "cam.ptr, "
      "dp.dow, "
      "dp.start_minute, "
      "dp.end_minute "
      "FROM %s AS cbna "
      "INNER JOIN campaigns AS cam "
      "ON cam.campaign_id = cbna.campaign_id "
      "INNER JOIN segments AS s "
      "ON s.creative_set_id = cbna.creative_set_id "
      "INNER JOIN creative_ads AS ca "
      "ON ca.creative_instance_id = cbna.creative_instance_id "
      "INNER JOIN geo_targets AS gt "
      "ON gt.campaign_id = cbna.campaign_id "
      "INNER JOIN dayparts AS dp "
      "ON dp.campaign_id = cbna.campaign_id "
      "WHERE s.segment IN %s "
      "AND cbna.dimensions = '%s' "
      "AND %s BETWEEN cam.start_at_timestamp AND cam.end_at_timestamp",
      get_table_name().c_str(),
      BuildBindingParameterPlaceholder(segments.size()).c_str(),
      dimensions.c_str(), TimeAsTimestampString(base::Time::Now()).c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::READ;
  command->command = query;

  int index = 0;
  for (const auto& segment : segments) {
    BindString(command.get(), index, base::ToLowerASCII(segment));
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
      DBCommand::RecordBindingType::INT_TYPE,     // per_week
      DBCommand::RecordBindingType::INT_TYPE,     // per_month
      DBCommand::RecordBindingType::INT_TYPE,     // total_max
      DBCommand::RecordBindingType::STRING_TYPE,  // split_test_group
      DBCommand::RecordBindingType::STRING_TYPE,  // segment
      DBCommand::RecordBindingType::STRING_TYPE,  // geo_target
      DBCommand::RecordBindingType::STRING_TYPE,  // target_url
      DBCommand::RecordBindingType::STRING_TYPE,  // title
      DBCommand::RecordBindingType::STRING_TYPE,  // description
      DBCommand::RecordBindingType::STRING_TYPE,  // image_url
      DBCommand::RecordBindingType::STRING_TYPE,  // dimensions
      DBCommand::RecordBindingType::STRING_TYPE,  // cta_text
      DBCommand::RecordBindingType::DOUBLE_TYPE,  // ptr
      DBCommand::RecordBindingType::STRING_TYPE,  // dayparts->dow
      DBCommand::RecordBindingType::INT_TYPE,     // dayparts->start_minute
      DBCommand::RecordBindingType::INT_TYPE      // dayparts->end_minute
  };

  DBTransactionPtr transaction = DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&CreativeInlineContentAds::OnGetForSegments, this,
                std::placeholders::_1, segments, callback));
}

void CreativeInlineContentAds::GetAll(
    GetCreativeInlineContentAdsCallback callback) {
  const std::string query = base::StringPrintf(
      "SELECT "
      "cbna.creative_instance_id, "
      "cbna.creative_set_id, "
      "cbna.campaign_id, "
      "cam.start_at_timestamp, "
      "cam.end_at_timestamp, "
      "cam.daily_cap, "
      "cam.advertiser_id, "
      "cam.priority, "
      "ca.conversion, "
      "ca.per_day, "
      "ca.per_week, "
      "ca.per_month, "
      "ca.total_max, "
      "ca.split_test_group, "
      "s.segment, "
      "gt.geo_target, "
      "ca.target_url, "
      "cbna.title, "
      "cbna.description, "
      "cbna.image_url, "
      "cbna.dimensions, "
      "cbna.cta_text, "
      "cam.ptr, "
      "dp.dow, "
      "dp.start_minute, "
      "dp.end_minute "
      "FROM %s AS cbna "
      "INNER JOIN campaigns AS cam "
      "ON cam.campaign_id = cbna.campaign_id "
      "INNER JOIN segments AS s "
      "ON s.creative_set_id = cbna.creative_set_id "
      "INNER JOIN creative_ads AS ca "
      "ON ca.creative_instance_id = cbna.creative_instance_id "
      "INNER JOIN geo_targets AS gt "
      "ON gt.campaign_id = cbna.campaign_id "
      "INNER JOIN dayparts AS dp "
      "ON dp.campaign_id = cbna.campaign_id "
      "WHERE %s BETWEEN cam.start_at_timestamp AND cam.end_at_timestamp",
      get_table_name().c_str(),
      TimeAsTimestampString(base::Time::Now()).c_str());

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
      DBCommand::RecordBindingType::INT_TYPE,     // per_week
      DBCommand::RecordBindingType::INT_TYPE,     // per_month
      DBCommand::RecordBindingType::INT_TYPE,     // total_max
      DBCommand::RecordBindingType::STRING_TYPE,  // split_test_group
      DBCommand::RecordBindingType::STRING_TYPE,  // segment
      DBCommand::RecordBindingType::STRING_TYPE,  // geo_target
      DBCommand::RecordBindingType::STRING_TYPE,  // target_url
      DBCommand::RecordBindingType::STRING_TYPE,  // title
      DBCommand::RecordBindingType::STRING_TYPE,  // description
      DBCommand::RecordBindingType::STRING_TYPE,  // image_url
      DBCommand::RecordBindingType::STRING_TYPE,  // dimensions
      DBCommand::RecordBindingType::STRING_TYPE,  // cta_text
      DBCommand::RecordBindingType::DOUBLE_TYPE,  // ptr
      DBCommand::RecordBindingType::STRING_TYPE,  // dayparts->dow
      DBCommand::RecordBindingType::INT_TYPE,     // dayparts->start_minute
      DBCommand::RecordBindingType::INT_TYPE      // dayparts->end_minute
  };

  DBTransactionPtr transaction = DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction), std::bind(&CreativeInlineContentAds::OnGetAll,
                                        this, std::placeholders::_1, callback));
}

void CreativeInlineContentAds::set_batch_size(const int batch_size) {
  DCHECK_GT(batch_size, 0);

  batch_size_ = batch_size;
}

std::string CreativeInlineContentAds::get_table_name() const {
  return kTableName;
}

void CreativeInlineContentAds::Migrate(DBTransaction* transaction,
                                       const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 15: {
      MigrateToV15(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeInlineContentAds::InsertOrUpdate(
    DBTransaction* transaction,
    const CreativeInlineContentAdList& creative_inline_content_ads) {
  DCHECK(transaction);

  if (creative_inline_content_ads.empty()) {
    return;
  }

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::RUN;
  command->command =
      BuildInsertOrUpdateQuery(command.get(), creative_inline_content_ads);

  transaction->commands.push_back(std::move(command));
}

int CreativeInlineContentAds::BindParameters(
    DBCommand* command,
    const CreativeInlineContentAdList& creative_inline_content_ads) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& creative_inline_content_ad : creative_inline_content_ads) {
    BindString(command, index++,
               creative_inline_content_ad.creative_instance_id);
    BindString(command, index++, creative_inline_content_ad.creative_set_id);
    BindString(command, index++, creative_inline_content_ad.campaign_id);
    BindString(command, index++, creative_inline_content_ad.title);
    BindString(command, index++, creative_inline_content_ad.description);
    BindString(command, index++, creative_inline_content_ad.image_url);
    BindString(command, index++, creative_inline_content_ad.dimensions);
    BindString(command, index++, creative_inline_content_ad.cta_text);

    count++;
  }

  return count;
}

std::string CreativeInlineContentAds::BuildInsertOrUpdateQuery(
    DBCommand* command,
    const CreativeInlineContentAdList& creative_inline_content_ads) {
  const int count = BindParameters(command, creative_inline_content_ads);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(creative_instance_id, "
      "creative_set_id, "
      "campaign_id, "
      "title, "
      "description, "
      "image_url, "
      "dimensions, "
      "cta_text) VALUES %s",
      get_table_name().c_str(),
      BuildBindingParameterPlaceholders(8, count).c_str());
}

void CreativeInlineContentAds::OnGetForCreativeInstanceId(
    DBCommandResponsePtr response,
    const std::string& creative_instance_id,
    GetCreativeInlineContentAdCallback callback) {
  if (!response || response->status != DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get creative inline content ad");
    callback(Result::FAILED, creative_instance_id, {});
    return;
  }

  if (response->result->get_records().size() != 1) {
    BLOG(0, "Failed to get creative inline content ad");
    callback(Result::FAILED, creative_instance_id, {});
    return;
  }

  DBRecord* record = response->result->get_records().at(0).get();

  const CreativeInlineContentAdInfo creative_inline_content_ad =
      GetFromRecord(record);

  callback(Result::SUCCESS, creative_instance_id, creative_inline_content_ad);
}

void CreativeInlineContentAds::OnGetForSegments(
    DBCommandResponsePtr response,
    const SegmentList& segments,
    GetCreativeInlineContentAdsCallback callback) {
  if (!response || response->status != DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get creative inline content ads");
    callback(Result::FAILED, segments, {});
    return;
  }

  CreativeInlineContentAdList creative_inline_content_ads;

  for (const auto& record : response->result->get_records()) {
    const CreativeInlineContentAdInfo creative_inline_content_ad =
        GetFromRecord(record.get());

    creative_inline_content_ads.push_back(creative_inline_content_ad);
  }

  callback(Result::SUCCESS, segments, creative_inline_content_ads);
}

void CreativeInlineContentAds::OnGetAll(
    DBCommandResponsePtr response,
    GetCreativeInlineContentAdsCallback callback) {
  if (!response || response->status != DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get all creative inline content ads");
    callback(Result::FAILED, {}, {});
    return;
  }

  CreativeInlineContentAdList creative_inline_content_ads;

  SegmentList segments;

  for (const auto& record : response->result->get_records()) {
    const CreativeInlineContentAdInfo creative_inline_content_ad =
        GetFromRecord(record.get());

    creative_inline_content_ads.push_back(creative_inline_content_ad);

    segments.push_back(creative_inline_content_ad.segment);
  }

  std::sort(segments.begin(), segments.end());
  const auto iter = std::unique(segments.begin(), segments.end());
  segments.erase(iter, segments.end());

  callback(Result::SUCCESS, segments, creative_inline_content_ads);
}

CreativeInlineContentAdInfo CreativeInlineContentAds::GetFromRecord(
    DBRecord* record) const {
  CreativeInlineContentAdInfo creative_inline_content_ad;

  creative_inline_content_ad.creative_instance_id = ColumnString(record, 0);
  creative_inline_content_ad.creative_set_id = ColumnString(record, 1);
  creative_inline_content_ad.campaign_id = ColumnString(record, 2);
  creative_inline_content_ad.start_at_timestamp = ColumnInt64(record, 3);
  creative_inline_content_ad.end_at_timestamp = ColumnInt64(record, 4);
  creative_inline_content_ad.daily_cap = ColumnInt(record, 5);
  creative_inline_content_ad.advertiser_id = ColumnString(record, 6);
  creative_inline_content_ad.priority = ColumnInt(record, 7);
  creative_inline_content_ad.conversion = ColumnBool(record, 8);
  creative_inline_content_ad.per_day = ColumnInt(record, 9);
  creative_inline_content_ad.per_week = ColumnInt(record, 10);
  creative_inline_content_ad.per_month = ColumnInt(record, 11);
  creative_inline_content_ad.total_max = ColumnInt(record, 12);
  creative_inline_content_ad.split_test_group = ColumnString(record, 13);
  creative_inline_content_ad.segment = ColumnString(record, 14);
  creative_inline_content_ad.geo_targets.push_back(ColumnString(record, 15));
  creative_inline_content_ad.target_url = ColumnString(record, 16);
  creative_inline_content_ad.title = ColumnString(record, 17);
  creative_inline_content_ad.description = ColumnString(record, 18);
  creative_inline_content_ad.image_url = ColumnString(record, 19);
  creative_inline_content_ad.dimensions = ColumnString(record, 20);
  creative_inline_content_ad.cta_text = ColumnString(record, 21);
  creative_inline_content_ad.ptr = ColumnDouble(record, 22);

  CreativeDaypartInfo daypart;
  daypart.dow = ColumnString(record, 23);
  daypart.start_minute = ColumnInt(record, 24);
  daypart.end_minute = ColumnInt(record, 25);
  creative_inline_content_ad.dayparts.push_back(daypart);

  return creative_inline_content_ad;
}

void CreativeInlineContentAds::CreateTableV15(DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
      "(creative_instance_id TEXT NOT NULL PRIMARY KEY UNIQUE "
      "ON CONFLICT REPLACE, "
      "creative_set_id TEXT NOT NULL, "
      "campaign_id TEXT NOT NULL, "
      "title TEXT NOT NULL, "
      "description TEXT NOT NULL, "
      "image_url TEXT NOT NULL, "
      "dimensions TEXT NOT NULL, "
      "cta_text TEXT NOT NULL)",
      get_table_name().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void CreativeInlineContentAds::MigrateToV15(DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, get_table_name());

  CreateTableV15(transaction);
}

}  // namespace table
}  // namespace database
}  // namespace ads
