/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_table.h"

#include <map>
#include <utility>

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
#include "url/gurl.h"

namespace ads::database::table {

using CreativePromotedContentAdMap =
    std::map<std::string, CreativePromotedContentAdInfo>;

namespace {

constexpr char kTableName[] = "creative_promoted_content_ads";

constexpr int kDefaultBatchSize = 50;

int BindParameters(mojom::DBCommandInfo* command,
                   const CreativePromotedContentAdList& creative_ads) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindString(command, index++, creative_ad.creative_instance_id);
    BindString(command, index++, creative_ad.creative_set_id);
    BindString(command, index++, creative_ad.campaign_id);
    BindString(command, index++, creative_ad.title);
    BindString(command, index++, creative_ad.description);

    count++;
  }

  return count;
}

CreativePromotedContentAdInfo GetFromRecord(mojom::DBRecordInfo* record) {
  DCHECK(record);

  CreativePromotedContentAdInfo creative_ad;

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
  creative_ad.target_url = GURL(ColumnString(record, 16));
  creative_ad.title = ColumnString(record, 17);
  creative_ad.description = ColumnString(record, 18);
  creative_ad.ptr = ColumnDouble(record, 19);

  CreativeDaypartInfo daypart;
  daypart.dow = ColumnString(record, 20);
  daypart.start_minute = ColumnInt(record, 21);
  daypart.end_minute = ColumnInt(record, 22);
  creative_ad.dayparts.push_back(daypart);

  return creative_ad;
}

CreativePromotedContentAdMap GroupCreativeAdsFromResponse(
    mojom::DBCommandResponseInfoPtr response) {
  DCHECK(response);

  CreativePromotedContentAdMap creative_ads;

  for (const auto& record : response->result->get_records()) {
    const CreativePromotedContentAdInfo creative_ad =
        GetFromRecord(record.get());

    const auto iter = creative_ads.find(creative_ad.creative_instance_id);
    if (iter == creative_ads.cend()) {
      creative_ads.insert({creative_ad.creative_instance_id, creative_ad});
      continue;
    }

    // Creative instance already exists, so append new geo targets and dayparts
    // to the existing creative ad
    for (const auto& geo_target : creative_ad.geo_targets) {
      if (!base::Contains(iter->second.geo_targets, geo_target)) {
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

CreativePromotedContentAdList GetCreativeAdsFromResponse(
    mojom::DBCommandResponseInfoPtr response) {
  DCHECK(response);

  const CreativePromotedContentAdMap grouped_creative_ads =
      GroupCreativeAdsFromResponse(std::move(response));

  CreativePromotedContentAdList creative_ads;
  for (const auto& grouped_creative_ad : grouped_creative_ads) {
    const CreativePromotedContentAdInfo creative_ad =
        grouped_creative_ad.second;
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

void OnGetForCreativeInstanceId(const std::string& creative_instance_id,
                                GetCreativePromotedContentAdCallback callback,
                                mojom::DBCommandResponseInfoPtr response) {
  if (!response || response->status !=
                       mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get creative promoted content ad");
    std::move(callback).Run(/*success*/ false, creative_instance_id, {});
    return;
  }

  const CreativePromotedContentAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(response));

  if (creative_ads.size() != 1) {
    BLOG(0, "Failed to get creative promoted content ad");
    std::move(callback).Run(/*success*/ false, creative_instance_id, {});
    return;
  }

  const CreativePromotedContentAdInfo& creative_ad = creative_ads.front();

  std::move(callback).Run(/*success*/ true, creative_instance_id, creative_ad);
}

void OnGetForSegments(const SegmentList& segments,
                      GetCreativePromotedContentAdsCallback callback,
                      mojom::DBCommandResponseInfoPtr response) {
  if (!response || response->status !=
                       mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get creative promoted content ads");
    std::move(callback).Run(/*success*/ false, segments, {});
    return;
  }

  const CreativePromotedContentAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(response));

  std::move(callback).Run(/*success*/ true, segments, creative_ads);
}

void OnGetAll(GetCreativePromotedContentAdsCallback callback,
              mojom::DBCommandResponseInfoPtr response) {
  if (!response || response->status !=
                       mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get all creative new tab page ads");
    std::move(callback).Run(/*success*/ false, {}, {});
    return;
  }

  const CreativePromotedContentAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(response));

  const SegmentList segments = GetSegments(creative_ads);

  std::move(callback).Run(/*success*/ true, segments, creative_ads);
}

void MigrateToV24(mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  DropTable(transaction, "creative_promoted_content_ads");

  const std::string query =
      "CREATE TABLE creative_promoted_content_ads "
      "(creative_instance_id TEXT NOT NULL PRIMARY KEY UNIQUE "
      "ON CONFLICT REPLACE, "
      "creative_set_id TEXT NOT NULL, "
      "campaign_id TEXT NOT NULL, "
      "title TEXT NOT NULL, "
      "description TEXT NOT NULL)";

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace

CreativePromotedContentAds::CreativePromotedContentAds()
    : batch_size_(kDefaultBatchSize),
      campaigns_database_table_(std::make_unique<Campaigns>()),
      creative_ads_database_table_(std::make_unique<CreativeAds>()),
      dayparts_database_table_(std::make_unique<Dayparts>()),
      geo_targets_database_table_(std::make_unique<GeoTargets>()),
      segments_database_table_(std::make_unique<Segments>()) {}

CreativePromotedContentAds::~CreativePromotedContentAds() = default;

void CreativePromotedContentAds::Save(
    const CreativePromotedContentAdList& creative_ads,
    ResultCallback callback) {
  if (creative_ads.empty()) {
    std::move(callback).Run(/*success*/ true);
    return;
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  const std::vector<CreativePromotedContentAdList> batches =
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

void CreativePromotedContentAds::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(transaction.get(), GetTableName());

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void CreativePromotedContentAds::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativePromotedContentAdCallback callback) const {
  if (creative_instance_id.empty()) {
    std::move(callback).Run(/*success*/ false, creative_instance_id, {});
    return;
  }

  const std::string query = base::StringPrintf(
      "SELECT "
      "cpca.creative_instance_id, "
      "cpca.creative_set_id, "
      "cpca.campaign_id, "
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
      "cpca.title, "
      "cpca.description, "
      "cam.ptr, "
      "dp.dow, "
      "dp.start_minute, "
      "dp.end_minute "
      "FROM %s AS cpca "
      "INNER JOIN campaigns AS cam "
      "ON cam.campaign_id = cpca.campaign_id "
      "INNER JOIN segments AS s "
      "ON s.creative_set_id = cpca.creative_set_id "
      "INNER JOIN creative_ads AS ca "
      "ON ca.creative_instance_id = cpca.creative_instance_id "
      "INNER JOIN geo_targets AS gt "
      "ON gt.campaign_id = cpca.campaign_id "
      "INNER JOIN dayparts AS dp "
      "ON dp.campaign_id = cpca.campaign_id "
      "WHERE cpca.creative_instance_id = '%s'",
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
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // geo_target
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // target_url
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // title
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // description
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

void CreativePromotedContentAds::GetForSegments(
    const SegmentList& segments,
    GetCreativePromotedContentAdsCallback callback) const {
  if (segments.empty()) {
    std::move(callback).Run(/*success*/ true, segments, {});
    return;
  }

  const std::string query = base::StringPrintf(
      "SELECT "
      "cpca.creative_instance_id, "
      "cpca.creative_set_id, "
      "cpca.campaign_id, "
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
      "cpca.title, "
      "cpca.description, "
      "cam.ptr, "
      "dp.dow, "
      "dp.start_minute, "
      "dp.end_minute "
      "FROM %s AS cpca "
      "INNER JOIN campaigns AS cam "
      "ON cam.campaign_id = cpca.campaign_id "
      "INNER JOIN segments AS s "
      "ON s.creative_set_id = cpca.creative_set_id "
      "INNER JOIN creative_ads AS ca "
      "ON ca.creative_instance_id = cpca.creative_instance_id "
      "INNER JOIN geo_targets AS gt "
      "ON gt.campaign_id = cpca.campaign_id "
      "INNER JOIN dayparts AS dp "
      "ON dp.campaign_id = cpca.campaign_id "
      "WHERE s.segment IN %s "
      "AND %s BETWEEN cam.start_at_timestamp AND cam.end_at_timestamp",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholder(segments.size()).c_str(),
      TimeAsTimestampString(base::Time::Now()).c_str());

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
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // geo_target
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // target_url
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // title
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // description
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
      base::BindOnce(&OnGetForSegments, segments, std::move(callback)));
}

void CreativePromotedContentAds::GetAll(
    GetCreativePromotedContentAdsCallback callback) const {
  const std::string query = base::StringPrintf(
      "SELECT "
      "cpca.creative_instance_id, "
      "cpca.creative_set_id, "
      "cpca.campaign_id, "
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
      "cpca.title, "
      "cpca.description, "
      "cam.ptr, "
      "dp.dow, "
      "dp.start_minute, "
      "dp.end_minute "
      "FROM %s AS cpca "
      "INNER JOIN campaigns AS cam "
      "ON cam.campaign_id = cpca.campaign_id "
      "INNER JOIN segments AS s "
      "ON s.creative_set_id = cpca.creative_set_id "
      "INNER JOIN creative_ads AS ca "
      "ON ca.creative_instance_id = cpca.creative_instance_id "
      "INNER JOIN geo_targets AS gt "
      "ON gt.campaign_id = cpca.campaign_id "
      "INNER JOIN dayparts AS dp "
      "ON dp.campaign_id = cpca.campaign_id "
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
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // geo_target
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // target_url
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // title
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // description
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

std::string CreativePromotedContentAds::GetTableName() const {
  return kTableName;
}

void CreativePromotedContentAds::Migrate(mojom::DBTransactionInfo* transaction,
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

void CreativePromotedContentAds::InsertOrUpdate(
    mojom::DBTransactionInfo* transaction,
    const CreativePromotedContentAdList& creative_ads) {
  DCHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), creative_ads);

  transaction->commands.push_back(std::move(command));
}

std::string CreativePromotedContentAds::BuildInsertOrUpdateQuery(
    mojom::DBCommandInfo* command,
    const CreativePromotedContentAdList& creative_ads) const {
  DCHECK(command);

  const int count = BindParameters(command, creative_ads);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(creative_instance_id, "
      "creative_set_id, "
      "campaign_id, "
      "title, "
      "description) VALUES %s",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(5, count).c_str());
}

}  // namespace ads::database::table
