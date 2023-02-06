/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"

#include <map>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/internal/account/deposits/deposits_database_table.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/containers/container_util.h"
#include "bat/ads/internal/common/database/database_bind_util.h"
#include "bat/ads/internal/common/database/database_column_util.h"
#include "bat/ads/internal/common/database/database_table_util.h"
#include "bat/ads/internal/common/database/database_transaction_util.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/common/time/time_formatting_util.h"
#include "bat/ads/internal/creatives/campaigns_database_table.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/creatives/creative_ads_database_table.h"
#include "bat/ads/internal/creatives/dayparts_database_table.h"
#include "bat/ads/internal/creatives/geo_targets_database_table.h"
#include "bat/ads/internal/creatives/segments_database_table.h"
#include "bat/ads/internal/segments/segment_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "url/gurl.h"

namespace ads::database::table {

using CreativeInlineContentAdMap =
    std::map<std::string, CreativeInlineContentAdInfo>;

namespace {

constexpr char kTableName[] = "creative_inline_content_ads";

constexpr int kDefaultBatchSize = 50;

int BindParameters(mojom::DBCommandInfo* command,
                   const CreativeInlineContentAdList& creative_ads) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindString(command, index++, creative_ad.creative_instance_id);
    BindString(command, index++, creative_ad.creative_set_id);
    BindString(command, index++, creative_ad.campaign_id);
    BindString(command, index++, creative_ad.title);
    BindString(command, index++, creative_ad.description);
    BindString(command, index++, creative_ad.image_url.spec());
    BindString(command, index++, creative_ad.dimensions);
    BindString(command, index++, creative_ad.cta_text);

    count++;
  }

  return count;
}

CreativeInlineContentAdInfo GetFromRecord(mojom::DBRecordInfo* record) {
  DCHECK(record);

  CreativeInlineContentAdInfo creative_ad;

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
  creative_ad.split_test_group = ColumnString(record, 14);
  creative_ad.segment = ColumnString(record, 15);
  creative_ad.geo_targets.insert(ColumnString(record, 16));
  creative_ad.target_url = GURL(ColumnString(record, 17));
  creative_ad.title = ColumnString(record, 18);
  creative_ad.description = ColumnString(record, 19);
  creative_ad.image_url = GURL(ColumnString(record, 20));
  creative_ad.dimensions = ColumnString(record, 21);
  creative_ad.cta_text = ColumnString(record, 22);
  creative_ad.ptr = ColumnDouble(record, 23);

  CreativeDaypartInfo daypart;
  daypart.dow = ColumnString(record, 24);
  daypart.start_minute = ColumnInt(record, 25);
  daypart.end_minute = ColumnInt(record, 26);
  creative_ad.dayparts.push_back(daypart);

  return creative_ad;
}

CreativeInlineContentAdMap GroupCreativeAdsFromResponse(
    mojom::DBCommandResponseInfoPtr response) {
  DCHECK(response);

  CreativeInlineContentAdMap creative_ads;

  for (const auto& record : response->result->get_records()) {
    const CreativeInlineContentAdInfo creative_ad = GetFromRecord(record.get());

    const auto iter = creative_ads.find(creative_ad.creative_instance_id);
    if (iter == creative_ads.cend()) {
      creative_ads.insert({creative_ad.creative_instance_id, creative_ad});
      continue;
    }

    // Creative instance already exists, so append new geo targets and dayparts
    // to the existing creative ad
    for (const auto& geo_target : creative_ad.geo_targets) {
      const auto geo_target_iter = iter->second.geo_targets.find(geo_target);
      if (geo_target_iter == iter->second.geo_targets.cend()) {
        iter->second.geo_targets.insert(geo_target);
      }
    }

    for (const auto& daypart : creative_ad.dayparts) {
      if (!base::Contains(iter->second.dayparts, daypart)) {
        iter->second.dayparts.push_back(daypart);
      }
    }
  }

  return creative_ads;
}

CreativeInlineContentAdList GetCreativeAdsFromResponse(
    mojom::DBCommandResponseInfoPtr response) {
  DCHECK(response);

  const CreativeInlineContentAdMap grouped_creative_ads =
      GroupCreativeAdsFromResponse(std::move(response));

  CreativeInlineContentAdList creative_ads;
  for (const auto& grouped_creative_ad : grouped_creative_ads) {
    const CreativeInlineContentAdInfo creative_ad = grouped_creative_ad.second;
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

void OnGetForCreativeInstanceId(const std::string& creative_instance_id,
                                GetCreativeInlineContentAdCallback callback,
                                mojom::DBCommandResponseInfoPtr response) {
  if (!response || response->status !=
                       mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get creative inline content ad");
    std::move(callback).Run(/*success*/ false, creative_instance_id, {});
    return;
  }

  const CreativeInlineContentAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(response));

  if (creative_ads.size() != 1) {
    BLOG(0, "Failed to get creative inline content ad");
    std::move(callback).Run(/*success*/ false, creative_instance_id, {});
    return;
  }

  const CreativeInlineContentAdInfo& creative_ad = creative_ads.front();

  std::move(callback).Run(/*success*/ true, creative_instance_id, creative_ad);
}

void OnGetForSegmentsAndDimensions(const SegmentList& segments,
                                   GetCreativeInlineContentAdsCallback callback,
                                   mojom::DBCommandResponseInfoPtr response) {
  if (!response || response->status !=
                       mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get creative inline content ads");
    std::move(callback).Run(/*success*/ false, segments, {});
    return;
  }

  const CreativeInlineContentAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(response));

  std::move(callback).Run(/*success*/ true, segments, creative_ads);
}

void OnGetForDimensions(
    GetCreativeInlineContentAdsForDimensionsCallback callback,
    mojom::DBCommandResponseInfoPtr response) {
  if (!response || response->status !=
                       mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get creative inline content ads");
    std::move(callback).Run(/*success*/ false, {});
    return;
  }

  const CreativeInlineContentAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(response));

  std::move(callback).Run(/*success*/ true, creative_ads);
}

void OnGetAll(GetCreativeInlineContentAdsCallback callback,
              mojom::DBCommandResponseInfoPtr response) {
  if (!response || response->status !=
                       mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get all creative inline content ads");
    std::move(callback).Run(/*success*/ false, {}, {});
    return;
  }

  const CreativeInlineContentAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(response));

  const SegmentList segments = GetSegments(creative_ads);

  std::move(callback).Run(/*success*/ true, segments, creative_ads);
}

void MigrateToV24(mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  DropTable(transaction, "creative_inline_content_ads");

  const std::string query =
      "CREATE TABLE creative_inline_content_ads "
      "(creative_instance_id TEXT NOT NULL PRIMARY KEY UNIQUE "
      "ON CONFLICT REPLACE, "
      "creative_set_id TEXT NOT NULL, "
      "campaign_id TEXT NOT NULL, "
      "title TEXT NOT NULL, "
      "description TEXT NOT NULL, "
      "image_url TEXT NOT NULL, "
      "dimensions TEXT NOT NULL, "
      "cta_text TEXT NOT NULL)";

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

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
    const CreativeInlineContentAdList& creative_ads,
    ResultCallback callback) {
  if (creative_ads.empty()) {
    std::move(callback).Run(/*success*/ true);
    return;
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  const std::vector<CreativeInlineContentAdList> batches =
      SplitVector(creative_ads, batch_size_);

  for (const auto& batch : batches) {
    InsertOrUpdate(transaction.get(), batch);

    const CreativeAdList creative_ads_batch(batch.cbegin(), batch.cend());
    campaigns_database_table_->InsertOrUpdate(transaction.get(),
                                              creative_ads_batch);
    creative_ads_database_table_->InsertOrUpdate(transaction.get(),
                                                 creative_ads_batch);
    dayparts_database_table_->InsertOrUpdate(transaction.get(),
                                             creative_ads_batch);
    deposits_database_table_->InsertOrUpdate(transaction.get(),
                                             creative_ads_batch);
    geo_targets_database_table_->InsertOrUpdate(transaction.get(),
                                                creative_ads_batch);
    segments_database_table_->InsertOrUpdate(transaction.get(),
                                             creative_ads_batch);
  }

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void CreativeInlineContentAds::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(transaction.get(), GetTableName());

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void CreativeInlineContentAds::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativeInlineContentAdCallback callback) const {
  if (creative_instance_id.empty()) {
    std::move(callback).Run(/*success*/ false, creative_instance_id, {});
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
      "ca.value, "
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
      GetTableName().c_str(), creative_instance_id.c_str());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // start_at
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // end_at
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // daily_cap
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // priority
      mojom::DBCommandInfo::RecordBindingType::BOOL_TYPE,    // conversion
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_day
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_week
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_month
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // total_max
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // split_test_group
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // geo_target
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // target_url
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // title
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // description
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // image_url
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // dimensions
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // cta_text
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // ptr
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // dayparts->dow
      mojom::DBCommandInfo::RecordBindingType::
          INT_TYPE,  // dayparts->start_minute
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE  // dayparts->end_minute
  };

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnGetForCreativeInstanceId, creative_instance_id,
                     std::move(callback)));
}

void CreativeInlineContentAds::GetForSegmentsAndDimensions(
    const SegmentList& segments,
    const std::string& dimensions,
    GetCreativeInlineContentAdsCallback callback) const {
  if (segments.empty() || dimensions.empty()) {
    std::move(callback).Run(/*success*/ true, segments, {});
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
      "ca.value, "
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
      GetTableName().c_str(),
      BuildBindingParameterPlaceholder(segments.size()).c_str(),
      dimensions.c_str(), TimeAsTimestampString(base::Time::Now()).c_str());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->command = query;

  int index = 0;
  for (const auto& segment : segments) {
    BindString(command.get(), index, base::ToLowerASCII(segment));
    index++;
  }

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // start_at
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // end_at
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // daily_cap
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // priority
      mojom::DBCommandInfo::RecordBindingType::BOOL_TYPE,    // conversion
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_day
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_week
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_month
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // total_max
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // split_test_group
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // geo_target
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // target_url
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // title
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // description
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // image_url
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // dimensions
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // cta_text
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // ptr
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // dayparts->dow
      mojom::DBCommandInfo::RecordBindingType::
          INT_TYPE,  // dayparts->start_minute
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE  // dayparts->end_minute
  };

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction), base::BindOnce(&OnGetForSegmentsAndDimensions,
                                             segments, std::move(callback)));
}

void CreativeInlineContentAds::GetForDimensions(
    const std::string& dimensions,
    GetCreativeInlineContentAdsForDimensionsCallback callback) const {
  if (dimensions.empty()) {
    std::move(callback).Run(/*success*/ true, {});
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
      "ca.value, "
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
      "AND cbna.dimensions = '%s' "
      "AND %s BETWEEN cam.start_at_timestamp AND cam.end_at_timestamp",
      GetTableName().c_str(), dimensions.c_str(),
      TimeAsTimestampString(base::Time::Now()).c_str());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // start_at
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // end_at
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // daily_cap
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // priority
      mojom::DBCommandInfo::RecordBindingType::BOOL_TYPE,    // conversion
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_day
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_week
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_month
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // total_max
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // split_test_group
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // geo_target
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // target_url
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // title
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // description
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // image_url
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // dimensions
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // cta_text
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // ptr
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // dayparts->dow
      mojom::DBCommandInfo::RecordBindingType::
          INT_TYPE,  // dayparts->start_minute
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE  // dayparts->end_minute
  };

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnGetForDimensions, std::move(callback)));
}

void CreativeInlineContentAds::GetAll(
    GetCreativeInlineContentAdsCallback callback) const {
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
      "ca.value, "
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
      GetTableName().c_str(), TimeAsTimestampString(base::Time::Now()).c_str());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // start_at
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // end_at
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // daily_cap
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // priority
      mojom::DBCommandInfo::RecordBindingType::BOOL_TYPE,    // conversion
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_day
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_week
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_month
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // total_max
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // split_test_group
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // geo_target
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // target_url
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // title
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // description
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // image_url
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // dimensions
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // cta_text
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // ptr
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // dayparts->dow
      mojom::DBCommandInfo::RecordBindingType::
          INT_TYPE,  // dayparts->start_minute
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE  // dayparts->end_minute
  };

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction), base::BindOnce(&OnGetAll, std::move(callback)));
}

std::string CreativeInlineContentAds::GetTableName() const {
  return kTableName;
}

void CreativeInlineContentAds::Migrate(mojom::DBTransactionInfo* transaction,
                                       const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 24: {
      MigrateToV24(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeInlineContentAds::InsertOrUpdate(
    mojom::DBTransactionInfo* transaction,
    const CreativeInlineContentAdList& creative_ads) {
  DCHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), creative_ads);

  transaction->commands.push_back(std::move(command));
}

std::string CreativeInlineContentAds::BuildInsertOrUpdateQuery(
    mojom::DBCommandInfo* command,
    const CreativeInlineContentAdList& creative_ads) const {
  DCHECK(command);

  const int count = BindParameters(command, creative_ads);

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
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(8, count).c_str());
}

}  // namespace ads::database::table
