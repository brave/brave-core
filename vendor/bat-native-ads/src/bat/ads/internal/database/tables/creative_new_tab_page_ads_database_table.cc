/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/creative_new_tab_page_ads_database_table.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/bundle/creative_ad_info_aliases.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/container_util.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/database/tables/campaigns_database_table.h"
#include "bat/ads/internal/database/tables/creative_ads_database_table.h"
#include "bat/ads/internal/database/tables/dayparts_database_table.h"
#include "bat/ads/internal/database/tables/geo_targets_database_table.h"
#include "bat/ads/internal/database/tables/segments_database_table.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/segments/segments_util.h"
#include "bat/ads/internal/time_formatting_util.h"

namespace ads {
namespace database {
namespace table {

namespace {

const char kTableName[] = "creative_new_tab_page_ads";

const int kDefaultBatchSize = 50;

int BindParameters(mojom::DBCommand* command,
                   const CreativeNewTabPageAdList& creative_ads) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindString(command, index++, creative_ad.creative_instance_id);
    BindString(command, index++, creative_ad.creative_set_id);
    BindString(command, index++, creative_ad.campaign_id);
    BindString(command, index++, creative_ad.company_name);
    BindString(command, index++, creative_ad.alt);

    count++;
  }

  return count;
}

CreativeNewTabPageAdInfo GetFromRecord(mojom::DBRecord* record) {
  DCHECK(record);

  CreativeNewTabPageAdInfo creative_ad;

  creative_ad.creative_instance_id = ColumnString(record, 0);
  creative_ad.creative_set_id = ColumnString(record, 1);
  creative_ad.campaign_id = ColumnString(record, 2);
  creative_ad.start_at = base::Time::FromDoubleT(ColumnDouble(record, 3));
  creative_ad.end_at = base::Time::FromDoubleT(ColumnDouble(record, 4));
  creative_ad.daily_cap = ColumnInt(record, 5);
  creative_ad.advertiser_id = ColumnString(record, 6);
  creative_ad.priority = ColumnInt(record, 7);
  creative_ad.conversion = ColumnBool(record, 8);
  creative_ad.per_day = ColumnInt(record, 9);
  creative_ad.per_week = ColumnInt(record, 10);
  creative_ad.per_month = ColumnInt(record, 11);
  creative_ad.total_max = ColumnInt(record, 12);
  creative_ad.value = ColumnDouble(record, 13);
  creative_ad.segment = ColumnString(record, 14);
  creative_ad.geo_targets.insert(ColumnString(record, 15));
  creative_ad.target_url = ColumnString(record, 16);
  creative_ad.company_name = ColumnString(record, 17);
  creative_ad.alt = ColumnString(record, 18);
  creative_ad.ptr = ColumnDouble(record, 19);

  CreativeDaypartInfo daypart;
  daypart.dow = ColumnString(record, 20);
  daypart.start_minute = ColumnInt(record, 21);
  daypart.end_minute = ColumnInt(record, 22);
  creative_ad.dayparts.push_back(daypart);

  return creative_ad;
}

CreativeNewTabPageAdMap GroupCreativeAdsFromResponse(
    mojom::DBCommandResponsePtr response) {
  DCHECK(response);

  CreativeNewTabPageAdMap creative_ads;

  for (const auto& record : response->result->get_records()) {
    const CreativeNewTabPageAdInfo& creative_ad = GetFromRecord(record.get());

    const auto iter = creative_ads.find(creative_ad.creative_instance_id);
    if (iter == creative_ads.end()) {
      creative_ads.insert({creative_ad.creative_instance_id, creative_ad});
      continue;
    }

    // Creative instance already exists, so append the geo targets and dayparts
    // to the existing creative ad
    iter->second.geo_targets.insert(creative_ad.geo_targets.begin(),
                                    creative_ad.geo_targets.end());

    iter->second.dayparts.insert(iter->second.dayparts.end(),
                                 creative_ad.dayparts.begin(),
                                 creative_ad.dayparts.end());
  }

  return creative_ads;
}

CreativeNewTabPageAdList GetCreativeAdsFromResponse(
    mojom::DBCommandResponsePtr response) {
  DCHECK(response);

  const CreativeNewTabPageAdMap& grouped_creative_ads =
      GroupCreativeAdsFromResponse(std::move(response));

  CreativeNewTabPageAdList creative_ads;
  for (const auto& grouped_creative_ad : grouped_creative_ads) {
    const CreativeNewTabPageAdInfo& creative_ad = grouped_creative_ad.second;
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

}  // namespace

CreativeNewTabPageAds::CreativeNewTabPageAds()
    : batch_size_(kDefaultBatchSize),
      campaigns_database_table_(std::make_unique<Campaigns>()),
      creative_ads_database_table_(std::make_unique<CreativeAds>()),
      dayparts_database_table_(std::make_unique<Dayparts>()),
      geo_targets_database_table_(std::make_unique<GeoTargets>()),
      segments_database_table_(std::make_unique<Segments>()) {}

CreativeNewTabPageAds::~CreativeNewTabPageAds() = default;

void CreativeNewTabPageAds::Save(const CreativeNewTabPageAdList& creative_ads,
                                 ResultCallback callback) {
  if (creative_ads.empty()) {
    callback(/* success */ true);
    return;
  }

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  const std::vector<CreativeNewTabPageAdList>& batches =
      SplitVector(creative_ads, batch_size_);

  for (const auto& batch : batches) {
    InsertOrUpdate(transaction.get(), batch);

    const CreativeAdList creative_ads(batch.cbegin(), batch.cend());
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
  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  util::Delete(transaction.get(), GetTableName());

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void CreativeNewTabPageAds::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativeNewTabPageAdCallback callback) {
  if (creative_instance_id.empty()) {
    callback(/* success */ false, creative_instance_id, {});
    return;
  }

  const std::string& query = base::StringPrintf(
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
      "ca.value, "
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
      GetTableName().c_str(), creative_instance_id.c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_instance_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // start_at
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // end_at
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // daily_cap
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // priority
      mojom::DBCommand::RecordBindingType::BOOL_TYPE,    // conversion
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // per_day
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // per_week
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // per_month
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // total_max
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // geo_target
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // target_url
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // company_name
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // alt
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // ptr
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // dayparts->dow
      mojom::DBCommand::RecordBindingType::INT_TYPE,  // dayparts->start_minute
      mojom::DBCommand::RecordBindingType::INT_TYPE   // dayparts->end_minute
  };

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
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
    callback(/* success */ true, segments, {});
    return;
  }

  const std::string& query = base::StringPrintf(
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
      "ca.value, "
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
      GetTableName().c_str(),
      BuildBindingParameterPlaceholder(segments.size()).c_str(),
      TimeAsTimestampString(base::Time::Now()).c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  int index = 0;
  for (const auto& segment : segments) {
    BindString(command.get(), index, base::ToLowerASCII(segment));
    index++;
  }

  command->record_bindings = {
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_instance_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // start_at
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // end_at
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // daily_cap
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // priority
      mojom::DBCommand::RecordBindingType::BOOL_TYPE,    // conversion
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // per_day
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // per_week
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // per_month
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // total_max
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // geo_target
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // target_url
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // company_name
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // alt
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // ptr
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // dayparts->dow
      mojom::DBCommand::RecordBindingType::INT_TYPE,  // dayparts->start_minute
      mojom::DBCommand::RecordBindingType::INT_TYPE   // dayparts->end_minute
  };

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&CreativeNewTabPageAds::OnGetForSegments, this,
                std::placeholders::_1, segments, callback));
}

void CreativeNewTabPageAds::GetAll(GetCreativeNewTabPageAdsCallback callback) {
  const std::string& query = base::StringPrintf(
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
      "ca.value, "
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
      GetTableName().c_str(), TimeAsTimestampString(base::Time::Now()).c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_instance_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // start_at
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // end_at
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // daily_cap
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // priority
      mojom::DBCommand::RecordBindingType::BOOL_TYPE,    // conversion
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // per_day
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // per_week
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // per_month
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // total_max
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // geo_target
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // target_url
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // company_name
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // alt
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // ptr
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // dayparts->dow
      mojom::DBCommand::RecordBindingType::INT_TYPE,  // dayparts->start_minute
      mojom::DBCommand::RecordBindingType::INT_TYPE   // dayparts->end_minute
  };

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction), std::bind(&CreativeNewTabPageAds::OnGetAll, this,
                                        std::placeholders::_1, callback));
}

std::string CreativeNewTabPageAds::GetTableName() const {
  return kTableName;
}

void CreativeNewTabPageAds::Migrate(mojom::DBTransaction* transaction,
                                    const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 16: {
      MigrateToV16(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeNewTabPageAds::InsertOrUpdate(
    mojom::DBTransaction* transaction,
    const CreativeNewTabPageAdList& creative_ads) {
  DCHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), creative_ads);

  transaction->commands.push_back(std::move(command));
}

std::string CreativeNewTabPageAds::BuildInsertOrUpdateQuery(
    mojom::DBCommand* command,
    const CreativeNewTabPageAdList& creative_ads) {
  DCHECK(command);

  const int count = BindParameters(command, creative_ads);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(creative_instance_id, "
      "creative_set_id, "
      "campaign_id, "
      "company_name, "
      "alt) VALUES %s",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(5, count).c_str());
}

void CreativeNewTabPageAds::OnGetForCreativeInstanceId(
    mojom::DBCommandResponsePtr response,
    const std::string& creative_instance_id,
    GetCreativeNewTabPageAdCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get creative new tab page ad");
    callback(/* success */ false, creative_instance_id, {});
    return;
  }

  const CreativeNewTabPageAdList& creative_ads =
      GetCreativeAdsFromResponse(std::move(response));

  if (creative_ads.size() != 1) {
    BLOG(0, "Failed to get creative new tab page ad");
    callback(/* success */ false, creative_instance_id, {});
    return;
  }

  const CreativeNewTabPageAdInfo& creative_ad = creative_ads.front();

  callback(/* success */ true, creative_instance_id, creative_ad);
}

void CreativeNewTabPageAds::OnGetForSegments(
    mojom::DBCommandResponsePtr response,
    const SegmentList& segments,
    GetCreativeNewTabPageAdsCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get creative new tab page ads");
    callback(/* success */ false, segments, {});
    return;
  }

  const CreativeNewTabPageAdList& creative_ads =
      GetCreativeAdsFromResponse(std::move(response));

  callback(/* success */ true, segments, creative_ads);
}

void CreativeNewTabPageAds::OnGetAll(
    mojom::DBCommandResponsePtr response,
    GetCreativeNewTabPageAdsCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get all creative new tab page ads");
    callback(/* success */ false, {}, {});
    return;
  }

  const CreativeNewTabPageAdList& creative_ads =
      GetCreativeAdsFromResponse(std::move(response));

  const SegmentList& segments = GetSegments(creative_ads);

  callback(/* success */ true, segments, creative_ads);
}

void CreativeNewTabPageAds::MigrateToV16(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, "creative_new_tab_page_ads");

  const std::string& query =
      "CREATE TABLE creative_new_tab_page_ads "
      "(creative_instance_id TEXT NOT NULL PRIMARY KEY UNIQUE "
      "ON CONFLICT REPLACE, "
      "creative_set_id TEXT NOT NULL, "
      "campaign_id TEXT NOT NULL, "
      "company_name TEXT NOT NULL, "
      "alt TEXT NOT NULL)";

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace table
}  // namespace database
}  // namespace ads
