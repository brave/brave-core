/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"

#include <cstddef>
#include <map>
#include <utility>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/containers/container_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads::database::table {

using CreativeNewTabPageAdMap =
    std::map</*creative_ad_uuid*/ std::string, CreativeNewTabPageAdInfo>;

namespace {

constexpr char kTableName[] = "creative_new_tab_page_ads";

constexpr int kDefaultBatchSize = 50;

void BindRecords(mojom::DBCommandInfo* const command) {
  CHECK(command);

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommandInfo::RecordBindingType::INT64_TYPE,   // start_at
      mojom::DBCommandInfo::RecordBindingType::INT64_TYPE,   // end_at
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // daily_cap
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // priority
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_day
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_week
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_month
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // total_max
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // geo_target
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // target_url
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // company_name
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // image_url
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // alt
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // ptr
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // dayparts->days_of_week
      mojom::DBCommandInfo::RecordBindingType::
          INT_TYPE,  // dayparts->start_minute
      mojom::DBCommandInfo::RecordBindingType::
          INT_TYPE,  // dayparts->end_minute
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_new_tab_page_ad_wallpapers->image_url
      mojom::DBCommandInfo::RecordBindingType::
          INT_TYPE,  // creative_new_tab_page_ad_wallpapers->focal_point->x
      mojom::DBCommandInfo::RecordBindingType::
          INT_TYPE  // creative_new_tab_page_ad_wallpapers->focal_point->y
  };
}

size_t BindParameters(mojom::DBCommandInfo* command,
                      const CreativeNewTabPageAdList& creative_ads) {
  CHECK(command);

  size_t count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindString(command, index++, creative_ad.creative_instance_id);
    BindString(command, index++, creative_ad.creative_set_id);
    BindString(command, index++, creative_ad.campaign_id);
    BindString(command, index++, creative_ad.company_name);
    BindString(command, index++, creative_ad.image_url.spec());
    BindString(command, index++, creative_ad.alt);

    ++count;
  }

  return count;
}

CreativeNewTabPageAdInfo GetFromRecord(mojom::DBRecordInfo* const record) {
  CHECK(record);

  CreativeNewTabPageAdInfo creative_ad;

  creative_ad.creative_instance_id = ColumnString(record, 0);
  creative_ad.creative_set_id = ColumnString(record, 1);
  creative_ad.campaign_id = ColumnString(record, 2);
  creative_ad.start_at = ToTimeFromChromeTimestamp(ColumnInt64(record, 3));
  creative_ad.end_at = ToTimeFromChromeTimestamp(ColumnInt64(record, 4));
  creative_ad.daily_cap = ColumnInt(record, 5);
  creative_ad.advertiser_id = ColumnString(record, 6);
  creative_ad.priority = ColumnInt(record, 7);
  creative_ad.per_day = ColumnInt(record, 8);
  creative_ad.per_week = ColumnInt(record, 9);
  creative_ad.per_month = ColumnInt(record, 10);
  creative_ad.total_max = ColumnInt(record, 11);
  creative_ad.value = ColumnDouble(record, 12);
  creative_ad.segment = ColumnString(record, 13);
  creative_ad.geo_targets.insert(ColumnString(record, 14));
  creative_ad.target_url = GURL(ColumnString(record, 15));
  creative_ad.company_name = ColumnString(record, 16);
  creative_ad.image_url = GURL(ColumnString(record, 17));
  creative_ad.alt = ColumnString(record, 18);
  creative_ad.pass_through_rate = ColumnDouble(record, 19);

  CreativeDaypartInfo daypart;
  daypart.days_of_week = ColumnString(record, 20);
  daypart.start_minute = ColumnInt(record, 21);
  daypart.end_minute = ColumnInt(record, 22);
  creative_ad.dayparts.push_back(daypart);

  CreativeNewTabPageAdWallpaperInfo wallpaper;
  wallpaper.image_url = GURL(ColumnString(record, 23));
  wallpaper.focal_point.x = ColumnInt(record, 24);
  wallpaper.focal_point.y = ColumnInt(record, 25);
  creative_ad.wallpapers.push_back(wallpaper);

  return creative_ad;
}

CreativeNewTabPageAdList GetCreativeAdsFromResponse(
    mojom::DBCommandResponseInfoPtr command_response) {
  CHECK(command_response);
  CHECK(command_response->result);

  CreativeNewTabPageAdMap creative_ads;

  for (const auto& record : command_response->result->get_records()) {
    const CreativeNewTabPageAdInfo creative_ad = GetFromRecord(&*record);

    const std::string uuid =
        base::StrCat({creative_ad.creative_instance_id, creative_ad.segment});
    const auto iter = creative_ads.find(uuid);
    if (iter == creative_ads.cend()) {
      creative_ads.insert({uuid, creative_ad});
      continue;
    }

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

    for (const auto& wallpaper : creative_ad.wallpapers) {
      if (!base::Contains(iter->second.wallpapers, wallpaper)) {
        iter->second.wallpapers.push_back(wallpaper);
      }
    }
  }

  CreativeNewTabPageAdList normalized_creative_ads;
  for (const auto& [_, creative_ad] : creative_ads) {
    normalized_creative_ads.push_back(creative_ad);
  }

  return normalized_creative_ads;
}

void GetForCreativeInstanceIdCallback(
    const std::string& creative_instance_id,
    GetCreativeNewTabPageAdCallback callback,
    mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get creative new tab page ad");

    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ads=*/{});
  }

  const CreativeNewTabPageAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(command_response));

  if (creative_ads.size() != 1) {
    BLOG(0, "Failed to get creative new tab page ad");

    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ads=*/{});
  }

  const CreativeNewTabPageAdInfo& creative_ad = creative_ads.front();

  std::move(callback).Run(/*success=*/true, creative_instance_id, creative_ad);
}

void GetForSegmentsCallback(const SegmentList& segments,
                            GetCreativeNewTabPageAdsCallback callback,
                            mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get creative new tab page ads");

    return std::move(callback).Run(/*success=*/false, segments,
                                   /*creative_ads=*/{});
  }

  const CreativeNewTabPageAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(command_response));

  std::move(callback).Run(/*success=*/true, segments, creative_ads);
}

void GetAllCallback(GetCreativeNewTabPageAdsCallback callback,
                    mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get all creative new tab page ads");

    return std::move(callback).Run(/*success=*/false, /*segments=*/{},
                                   /*creative_ads=*/{});
  }

  const CreativeNewTabPageAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(command_response));

  const SegmentList segments = GetSegments(creative_ads);

  std::move(callback).Run(/*success=*/true, segments, creative_ads);
}

}  // namespace

CreativeNewTabPageAds::CreativeNewTabPageAds()
    : batch_size_(kDefaultBatchSize) {}

CreativeNewTabPageAds::~CreativeNewTabPageAds() = default;

void CreativeNewTabPageAds::Save(const CreativeNewTabPageAdList& creative_ads,
                                 ResultCallback callback) {
  if (creative_ads.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  const std::vector<CreativeNewTabPageAdList> batches =
      SplitVector(creative_ads, batch_size_);

  for (const auto& batch : batches) {
    InsertOrUpdate(&*transaction, batch);

    const CreativeAdList creative_ads_batch(batch.cbegin(), batch.cend());
    campaigns_database_table_.InsertOrUpdate(&*transaction, creative_ads_batch);
    creative_ads_database_table_.InsertOrUpdate(&*transaction,
                                                creative_ads_batch);
    creative_new_tab_page_ad_wallpapers_database_table_.InsertOrUpdate(
        &*transaction, batch);
    dayparts_database_table_.InsertOrUpdate(&*transaction, creative_ads_batch);
    deposits_database_table_.InsertOrUpdate(&*transaction, creative_ads_batch);
    geo_targets_database_table_.InsertOrUpdate(&*transaction,
                                               creative_ads_batch);
    segments_database_table_.InsertOrUpdate(&*transaction, creative_ads_batch);
  }

  RunTransaction(std::move(transaction), std::move(callback));
}

void CreativeNewTabPageAds::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(&*transaction, GetTableName());

  RunTransaction(std::move(transaction), std::move(callback));
}

void CreativeNewTabPageAds::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativeNewTabPageAdCallback callback) const {
  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ads=*/{});
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_new_tab_page_ad.creative_instance_id,
            creative_new_tab_page_ad.creative_set_id,
            creative_new_tab_page_ad.campaign_id,
            campaigns.start_at,
            campaigns.end_at,
            campaigns.daily_cap,
            campaigns.advertiser_id,
            campaigns.priority,
            creative_ads.per_day,
            creative_ads.per_week,
            creative_ads.per_month,
            creative_ads.total_max,
            creative_ads.value,
            segments.segment,
            geo_targets.geo_target,
            creative_ads.target_url,
            creative_new_tab_page_ad.company_name,
            creative_new_tab_page_ad.image_url,
            creative_new_tab_page_ad.alt,
            campaigns.ptr,
            dayparts.days_of_week,
            dayparts.start_minute,
            dayparts.end_minute,
            creative_new_tab_page_ad_wallpapers.image_url,
            creative_new_tab_page_ad_wallpapers.focal_point_x,
            creative_new_tab_page_ad_wallpapers.focal_point_y
          FROM
            $1 AS creative_new_tab_page_ad
            INNER JOIN campaigns ON campaigns.id = creative_new_tab_page_ad.campaign_id
            INNER JOIN creative_ads ON creative_ads.creative_instance_id = creative_new_tab_page_ad.creative_instance_id
            INNER JOIN creative_new_tab_page_ad_wallpapers ON creative_new_tab_page_ad_wallpapers.creative_instance_id = creative_new_tab_page_ad.creative_instance_id
            INNER JOIN dayparts ON dayparts.campaign_id = creative_new_tab_page_ad.campaign_id
            INNER JOIN geo_targets ON geo_targets.campaign_id = creative_new_tab_page_ad.campaign_id
            INNER JOIN segments ON segments.creative_set_id = creative_new_tab_page_ad.creative_set_id
          WHERE
            creative_new_tab_page_ad.creative_instance_id = '$2';)",
      {GetTableName(), creative_instance_id}, nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetForCreativeInstanceIdCallback,
                                  creative_instance_id, std::move(callback)));
}

void CreativeNewTabPageAds::GetForSegments(
    const SegmentList& segments,
    GetCreativeNewTabPageAdsCallback callback) const {
  if (segments.empty()) {
    return std::move(callback).Run(/*success=*/true, segments,
                                   /*creative_ads=*/{});
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_new_tab_page_ad.creative_instance_id,
            creative_new_tab_page_ad.creative_set_id,
            creative_new_tab_page_ad.campaign_id,
            campaigns.start_at,
            campaigns.end_at,
            campaigns.daily_cap,
            campaigns.advertiser_id,
            campaigns.priority,
            creative_ads.per_day,
            creative_ads.per_week,
            creative_ads.per_month,
            creative_ads.total_max,
            creative_ads.value,
            segments.segment,
            geo_targets.geo_target,
            creative_ads.target_url,
            creative_new_tab_page_ad.company_name,
            creative_new_tab_page_ad.image_url,
            creative_new_tab_page_ad.alt,
            campaigns.ptr,
            dayparts.days_of_week,
            dayparts.start_minute,
            dayparts.end_minute,
            creative_new_tab_page_ad_wallpapers.image_url,
            creative_new_tab_page_ad_wallpapers.focal_point_x,
            creative_new_tab_page_ad_wallpapers.focal_point_y
          FROM
            $1 AS creative_new_tab_page_ad
            INNER JOIN campaigns ON campaigns.id = creative_new_tab_page_ad.campaign_id
            INNER JOIN creative_ads ON creative_ads.creative_instance_id = creative_new_tab_page_ad.creative_instance_id
            INNER JOIN creative_new_tab_page_ad_wallpapers ON creative_new_tab_page_ad_wallpapers.creative_instance_id = creative_new_tab_page_ad.creative_instance_id
            INNER JOIN dayparts ON dayparts.campaign_id = creative_new_tab_page_ad.campaign_id
            INNER JOIN geo_targets ON geo_targets.campaign_id = creative_new_tab_page_ad.campaign_id
            INNER JOIN segments ON segments.creative_set_id = creative_new_tab_page_ad.creative_set_id
          WHERE
            segments.segment IN $2
            AND $3 BETWEEN campaigns.start_at AND campaigns.end_at;)",
      {GetTableName(),
       BuildBindingParameterPlaceholder(/*parameters_count=*/segments.size()),
       base::NumberToString(ToChromeTimestampFromTime(base::Time::Now()))},
      nullptr);
  BindRecords(&*command);

  int index = 0;
  for (const auto& segment : segments) {
    BindString(&*command, index++, segment);
  }

  transaction->commands.push_back(std::move(command));

  RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&GetForSegmentsCallback, segments, std::move(callback)));
}

void CreativeNewTabPageAds::GetForActiveCampaigns(
    GetCreativeNewTabPageAdsCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_new_tab_page_ad.creative_instance_id,
            creative_new_tab_page_ad.creative_set_id,
            creative_new_tab_page_ad.campaign_id,
            campaigns.start_at,
            campaigns.end_at,
            campaigns.daily_cap,
            campaigns.advertiser_id,
            campaigns.priority,
            creative_ads.per_day,
            creative_ads.per_week,
            creative_ads.per_month,
            creative_ads.total_max,
            creative_ads.value,
            segments.segment,
            geo_targets.geo_target,
            creative_ads.target_url,
            creative_new_tab_page_ad.company_name,
            creative_new_tab_page_ad.image_url,
            creative_new_tab_page_ad.alt,
            campaigns.ptr,
            dayparts.days_of_week,
            dayparts.start_minute,
            dayparts.end_minute,
            creative_new_tab_page_ad_wallpapers.image_url,
            creative_new_tab_page_ad_wallpapers.focal_point_x,
            creative_new_tab_page_ad_wallpapers.focal_point_y
          FROM
            $1 AS creative_new_tab_page_ad
            INNER JOIN campaigns ON campaigns.id = creative_new_tab_page_ad.campaign_id
            INNER JOIN creative_ads ON creative_ads.creative_instance_id = creative_new_tab_page_ad.creative_instance_id
            INNER JOIN creative_new_tab_page_ad_wallpapers ON creative_new_tab_page_ad_wallpapers.creative_instance_id = creative_new_tab_page_ad.creative_instance_id
            INNER JOIN dayparts ON dayparts.campaign_id = creative_new_tab_page_ad.campaign_id
            INNER JOIN geo_targets ON geo_targets.campaign_id = creative_new_tab_page_ad.campaign_id
            INNER JOIN segments ON segments.creative_set_id = creative_new_tab_page_ad.creative_set_id
          WHERE
            $2 BETWEEN campaigns.start_at AND campaigns.end_at;)",
      {GetTableName(),
       base::NumberToString(ToChromeTimestampFromTime(base::Time::Now()))},
      nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetAllCallback, std::move(callback)));
}

std::string CreativeNewTabPageAds::GetTableName() const {
  return kTableName;
}

void CreativeNewTabPageAds::Create(
    mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          CREATE TABLE creative_new_tab_page_ads (
            creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
            creative_set_id TEXT NOT NULL,
            campaign_id TEXT NOT NULL,
            company_name TEXT NOT NULL,
            image_url TEXT NOT NULL,
            alt TEXT NOT NULL
          );)";
  transaction->commands.push_back(std::move(command));
}

void CreativeNewTabPageAds::Migrate(mojom::DBTransactionInfo* transaction,
                                    const int to_version) {
  CHECK(transaction);

  switch (to_version) {
    case 35: {
      MigrateToV35(transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeNewTabPageAds::MigrateToV35(
    mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // We can safely recreate the table because it will be repopulated after
  // downloading the catalog.
  DropTable(transaction, GetTableName());
  Create(transaction);
}

void CreativeNewTabPageAds::InsertOrUpdate(
    mojom::DBTransactionInfo* transaction,
    const CreativeNewTabPageAdList& creative_ads) {
  CHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, creative_ads);
  transaction->commands.push_back(std::move(command));
}

std::string CreativeNewTabPageAds::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const CreativeNewTabPageAdList& creative_ads) const {
  CHECK(command);

  const size_t binded_parameters_count = BindParameters(command, creative_ads);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_instance_id,
            creative_set_id,
            campaign_id,
            company_name,
            image_url,
            alt
          ) VALUES $2;)",
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count=*/6, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
