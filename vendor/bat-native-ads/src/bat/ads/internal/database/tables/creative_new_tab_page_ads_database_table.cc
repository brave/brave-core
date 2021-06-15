/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/creative_new_tab_page_ads_database_table.h"

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

const char kTableName[] = "creative_new_tab_page_ads";

const int kDefaultBatchSize = 50;

}  // namespace

CreativeNewTabPageAds::CreativeNewTabPageAds()
    : batch_size_(kDefaultBatchSize),
      campaigns_database_table_(std::make_unique<Campaigns>()),
      creative_ads_database_table_(std::make_unique<CreativeAds>()),
      dayparts_database_table_(std::make_unique<Dayparts>()),
      geo_targets_database_table_(std::make_unique<GeoTargets>()),
      segments_database_table_(std::make_unique<Segments>()) {}

CreativeNewTabPageAds::~CreativeNewTabPageAds() = default;

void CreativeNewTabPageAds::Save(
    const CreativeNewTabPageAdList& creative_new_tab_page_ads,
    ResultCallback callback) {
  if (creative_new_tab_page_ads.empty()) {
    callback(Result::SUCCESS);
    return;
  }

  DBTransactionPtr transaction = DBTransaction::New();

  const std::vector<CreativeNewTabPageAdList> batches =
      SplitVector(creative_new_tab_page_ads, batch_size_);

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

void CreativeNewTabPageAds::Delete(ResultCallback callback) {
  DBTransactionPtr transaction = DBTransaction::New();

  util::Delete(transaction.get(), get_table_name());

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void CreativeNewTabPageAds::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativeNewTabPageAdCallback callback) {
  CreativeNewTabPageAdInfo creative_new_tab_page_ad;

  if (creative_instance_id.empty()) {
    callback(Result::FAILED, creative_instance_id, creative_new_tab_page_ad);
    return;
  }

  const std::string query = base::StringPrintf(
      "SELECT "
      "cntpa.creative_instance_id, "
      "cntpa.creative_set_id, "
      "cntpa.campaign_id, "
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
      "s.segment, "
      "gt.geo_target, "
      "ca.target_url, "
      "cntpa.company_name, "
      "cntpa.alt, "
      "cam.ptr, "
      "dp.dow, "
      "dp.start_minute, "
      "dp.end_minute "
      "FROM %s AS cntpa "
      "INNER JOIN campaigns AS cam "
      "ON cam.campaign_id = cntpa.campaign_id "
      "INNER JOIN segments AS s "
      "ON s.creative_set_id = cntpa.creative_set_id "
      "INNER JOIN creative_ads AS ca "
      "ON ca.creative_instance_id = cntpa.creative_instance_id "
      "INNER JOIN geo_targets AS gt "
      "ON gt.campaign_id = cntpa.campaign_id "
      "INNER JOIN dayparts AS dp "
      "ON dp.campaign_id = cntpa.campaign_id "
      "WHERE cntpa.creative_instance_id = '%s'",
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
      DBCommand::RecordBindingType::STRING_TYPE,  // segment
      DBCommand::RecordBindingType::STRING_TYPE,  // geo_target
      DBCommand::RecordBindingType::STRING_TYPE,  // target_url
      DBCommand::RecordBindingType::STRING_TYPE,  // company_name
      DBCommand::RecordBindingType::STRING_TYPE,  // alt
      DBCommand::RecordBindingType::DOUBLE_TYPE,  // ptr
      DBCommand::RecordBindingType::STRING_TYPE,  // dayparts->dow
      DBCommand::RecordBindingType::INT_TYPE,     // dayparts->start_minute
      DBCommand::RecordBindingType::INT_TYPE      // dayparts->end_minute
  };

  DBTransactionPtr transaction = DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&CreativeNewTabPageAds::OnGetForCreativeInstanceId, this,
                std::placeholders::_1, creative_instance_id, callback));
}

void CreativeNewTabPageAds::GetForSegments(
    const SegmentList& segments,
    GetCreativeNewTabPageAdsCallback callback) {
  if (segments.empty()) {
    callback(Result::SUCCESS, segments, {});
    return;
  }

  const std::string query = base::StringPrintf(
      "SELECT "
      "cntpa.creative_instance_id, "
      "cntpa.creative_set_id, "
      "cntpa.campaign_id, "
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
      "s.segment, "
      "gt.geo_target, "
      "ca.target_url, "
      "cntpa.company_name, "
      "cntpa.alt, "
      "cam.ptr, "
      "dp.dow, "
      "dp.start_minute, "
      "dp.end_minute "
      "FROM %s AS cntpa "
      "INNER JOIN campaigns AS cam "
      "ON cam.campaign_id = cntpa.campaign_id "
      "INNER JOIN segments AS s "
      "ON s.creative_set_id = cntpa.creative_set_id "
      "INNER JOIN creative_ads AS ca "
      "ON ca.creative_instance_id = cntpa.creative_instance_id "
      "INNER JOIN geo_targets AS gt "
      "ON gt.campaign_id = cntpa.campaign_id "
      "INNER JOIN dayparts AS dp "
      "ON dp.campaign_id = cntpa.campaign_id "
      "WHERE s.segment IN %s "
      "AND %s BETWEEN cam.start_at_timestamp AND cam.end_at_timestamp",
      get_table_name().c_str(),
      BuildBindingParameterPlaceholder(segments.size()).c_str(),
      TimeAsTimestampString(base::Time::Now()).c_str());

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
      DBCommand::RecordBindingType::STRING_TYPE,  // segment
      DBCommand::RecordBindingType::STRING_TYPE,  // geo_target
      DBCommand::RecordBindingType::STRING_TYPE,  // target_url
      DBCommand::RecordBindingType::STRING_TYPE,  // company_name
      DBCommand::RecordBindingType::STRING_TYPE,  // alt
      DBCommand::RecordBindingType::DOUBLE_TYPE,  // ptr
      DBCommand::RecordBindingType::STRING_TYPE,  // dayparts->dow
      DBCommand::RecordBindingType::INT_TYPE,     // dayparts->start_minute
      DBCommand::RecordBindingType::INT_TYPE      // dayparts->end_minute
  };

  DBTransactionPtr transaction = DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&CreativeNewTabPageAds::OnGetForSegments, this,
                std::placeholders::_1, segments, callback));
}

void CreativeNewTabPageAds::GetAll(GetCreativeNewTabPageAdsCallback callback) {
  const std::string query = base::StringPrintf(
      "SELECT "
      "cntpa.creative_instance_id, "
      "cntpa.creative_set_id, "
      "cntpa.campaign_id, "
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
      "s.segment, "
      "gt.geo_target, "
      "ca.target_url, "
      "cntpa.company_name, "
      "cntpa.alt, "
      "cam.ptr, "
      "dp.dow, "
      "dp.start_minute, "
      "dp.end_minute "
      "FROM %s AS cntpa "
      "INNER JOIN campaigns AS cam "
      "ON cam.campaign_id = cntpa.campaign_id "
      "INNER JOIN segments AS s "
      "ON s.creative_set_id = cntpa.creative_set_id "
      "INNER JOIN creative_ads AS ca "
      "ON ca.creative_instance_id = cntpa.creative_instance_id "
      "INNER JOIN geo_targets AS gt "
      "ON gt.campaign_id = cntpa.campaign_id "
      "INNER JOIN dayparts AS dp "
      "ON dp.campaign_id = cntpa.campaign_id "
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
      DBCommand::RecordBindingType::STRING_TYPE,  // segment
      DBCommand::RecordBindingType::STRING_TYPE,  // geo_target
      DBCommand::RecordBindingType::STRING_TYPE,  // target_url
      DBCommand::RecordBindingType::STRING_TYPE,  // company_name
      DBCommand::RecordBindingType::STRING_TYPE,  // alt
      DBCommand::RecordBindingType::DOUBLE_TYPE,  // ptr
      DBCommand::RecordBindingType::STRING_TYPE,  // dayparts->dow
      DBCommand::RecordBindingType::INT_TYPE,     // dayparts->start_minute
      DBCommand::RecordBindingType::INT_TYPE      // dayparts->end_minute
  };

  DBTransactionPtr transaction = DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction), std::bind(&CreativeNewTabPageAds::OnGetAll, this,
                                        std::placeholders::_1, callback));
}

void CreativeNewTabPageAds::set_batch_size(const int batch_size) {
  DCHECK_GT(batch_size, 0);

  batch_size_ = batch_size;
}

std::string CreativeNewTabPageAds::get_table_name() const {
  return kTableName;
}

void CreativeNewTabPageAds::Migrate(DBTransaction* transaction,
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

void CreativeNewTabPageAds::InsertOrUpdate(
    DBTransaction* transaction,
    const CreativeNewTabPageAdList& creative_new_tab_page_ads) {
  DCHECK(transaction);

  if (creative_new_tab_page_ads.empty()) {
    return;
  }

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::RUN;
  command->command =
      BuildInsertOrUpdateQuery(command.get(), creative_new_tab_page_ads);

  transaction->commands.push_back(std::move(command));
}

int CreativeNewTabPageAds::BindParameters(
    DBCommand* command,
    const CreativeNewTabPageAdList& creative_new_tab_page_ads) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& creative_new_tab_page_ad : creative_new_tab_page_ads) {
    BindString(command, index++, creative_new_tab_page_ad.creative_instance_id);
    BindString(command, index++, creative_new_tab_page_ad.creative_set_id);
    BindString(command, index++, creative_new_tab_page_ad.campaign_id);
    BindString(command, index++, creative_new_tab_page_ad.company_name);
    BindString(command, index++, creative_new_tab_page_ad.alt);

    count++;
  }

  return count;
}

std::string CreativeNewTabPageAds::BuildInsertOrUpdateQuery(
    DBCommand* command,
    const CreativeNewTabPageAdList& creative_new_tab_page_ads) {
  const int count = BindParameters(command, creative_new_tab_page_ads);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(creative_instance_id, "
      "creative_set_id, "
      "campaign_id, "
      "company_name, "
      "alt) VALUES %s",
      get_table_name().c_str(),
      BuildBindingParameterPlaceholders(5, count).c_str());
}

void CreativeNewTabPageAds::OnGetForCreativeInstanceId(
    DBCommandResponsePtr response,
    const std::string& creative_instance_id,
    GetCreativeNewTabPageAdCallback callback) {
  if (!response || response->status != DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get creative new tab page ad");
    callback(Result::FAILED, creative_instance_id, {});
    return;
  }

  if (response->result->get_records().size() != 1) {
    BLOG(0, "Failed to get creative new tab page ad");
    callback(Result::FAILED, creative_instance_id, {});
    return;
  }

  DBRecord* record = response->result->get_records().at(0).get();

  const CreativeNewTabPageAdInfo creative_new_tab_page_ad =
      GetFromRecord(record);

  callback(Result::SUCCESS, creative_instance_id, creative_new_tab_page_ad);
}

void CreativeNewTabPageAds::OnGetForSegments(
    DBCommandResponsePtr response,
    const SegmentList& segments,
    GetCreativeNewTabPageAdsCallback callback) {
  if (!response || response->status != DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get creative new tab page ads");
    callback(Result::FAILED, segments, {});
    return;
  }

  CreativeNewTabPageAdList creative_new_tab_page_ads;

  for (const auto& record : response->result->get_records()) {
    const CreativeNewTabPageAdInfo creative_new_tab_page_ad =
        GetFromRecord(record.get());

    creative_new_tab_page_ads.push_back(creative_new_tab_page_ad);
  }

  callback(Result::SUCCESS, segments, creative_new_tab_page_ads);
}

void CreativeNewTabPageAds::OnGetAll(
    DBCommandResponsePtr response,
    GetCreativeNewTabPageAdsCallback callback) {
  if (!response || response->status != DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get all creative new tab page ads");
    callback(Result::FAILED, {}, {});
    return;
  }

  CreativeNewTabPageAdList creative_new_tab_page_ads;

  SegmentList segments;

  for (const auto& record : response->result->get_records()) {
    const CreativeNewTabPageAdInfo creative_new_tab_page_ad =
        GetFromRecord(record.get());

    creative_new_tab_page_ads.push_back(creative_new_tab_page_ad);

    segments.push_back(creative_new_tab_page_ad.segment);
  }

  std::sort(segments.begin(), segments.end());
  const auto iter = std::unique(segments.begin(), segments.end());
  segments.erase(iter, segments.end());

  callback(Result::SUCCESS, segments, creative_new_tab_page_ads);
}

CreativeNewTabPageAdInfo CreativeNewTabPageAds::GetFromRecord(
    DBRecord* record) const {
  CreativeNewTabPageAdInfo creative_new_tab_page_ad;

  creative_new_tab_page_ad.creative_instance_id = ColumnString(record, 0);
  creative_new_tab_page_ad.creative_set_id = ColumnString(record, 1);
  creative_new_tab_page_ad.campaign_id = ColumnString(record, 2);
  creative_new_tab_page_ad.start_at_timestamp = ColumnInt64(record, 3);
  creative_new_tab_page_ad.end_at_timestamp = ColumnInt64(record, 4);
  creative_new_tab_page_ad.daily_cap = ColumnInt(record, 5);
  creative_new_tab_page_ad.advertiser_id = ColumnString(record, 6);
  creative_new_tab_page_ad.priority = ColumnInt(record, 7);
  creative_new_tab_page_ad.conversion = ColumnBool(record, 8);
  creative_new_tab_page_ad.per_day = ColumnInt(record, 9);
  creative_new_tab_page_ad.per_week = ColumnInt(record, 10);
  creative_new_tab_page_ad.per_month = ColumnInt(record, 11);
  creative_new_tab_page_ad.total_max = ColumnInt(record, 12);
  creative_new_tab_page_ad.segment = ColumnString(record, 13);
  creative_new_tab_page_ad.geo_targets.push_back(ColumnString(record, 14));
  creative_new_tab_page_ad.target_url = ColumnString(record, 15);
  creative_new_tab_page_ad.company_name = ColumnString(record, 16);
  creative_new_tab_page_ad.alt = ColumnString(record, 17);
  creative_new_tab_page_ad.ptr = ColumnDouble(record, 18);

  CreativeDaypartInfo daypart;
  daypart.dow = ColumnString(record, 19);
  daypart.start_minute = ColumnInt(record, 20);
  daypart.end_minute = ColumnInt(record, 21);
  creative_new_tab_page_ad.dayparts.push_back(daypart);

  return creative_new_tab_page_ad;
}

void CreativeNewTabPageAds::CreateTableV15(DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
      "(creative_instance_id TEXT NOT NULL PRIMARY KEY UNIQUE "
      "ON CONFLICT REPLACE, "
      "creative_set_id TEXT NOT NULL, "
      "campaign_id TEXT NOT NULL, "
      "company_name TEXT NOT NULL, "
      "alt TEXT NOT NULL)",
      get_table_name().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void CreativeNewTabPageAds::MigrateToV15(DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, get_table_name());

  CreateTableV15(transaction);
}

}  // namespace table
}  // namespace database
}  // namespace ads
