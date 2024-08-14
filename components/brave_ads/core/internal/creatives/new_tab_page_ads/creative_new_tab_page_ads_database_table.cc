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
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"
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

void BindColumnTypes(mojom::DBStatementInfo* const mojom_statement) {
  CHECK(mojom_statement);

  mojom_statement->bind_column_types = {
      mojom::DBBindColumnType::kString,  // creative_instance_id
      mojom::DBBindColumnType::kString,  // creative_set_id
      mojom::DBBindColumnType::kString,  // campaign_id
      mojom::DBBindColumnType::kInt64,   // start_at
      mojom::DBBindColumnType::kInt64,   // end_at
      mojom::DBBindColumnType::kInt,     // daily_cap
      mojom::DBBindColumnType::kString,  // advertiser_id
      mojom::DBBindColumnType::kInt,     // priority
      mojom::DBBindColumnType::kInt,     // per_day
      mojom::DBBindColumnType::kInt,     // per_week
      mojom::DBBindColumnType::kInt,     // per_month
      mojom::DBBindColumnType::kInt,     // total_max
      mojom::DBBindColumnType::kDouble,  // value
      mojom::DBBindColumnType::kString,  // split_test_group
      mojom::DBBindColumnType::kString,  // segment
      mojom::DBBindColumnType::kString,  // geo_target
      mojom::DBBindColumnType::kString,  // target_url
      mojom::DBBindColumnType::kString,  // company_name
      mojom::DBBindColumnType::kString,  // image_url
      mojom::DBBindColumnType::kString,  // alt
      mojom::DBBindColumnType::kDouble,  // ptr
      mojom::DBBindColumnType::kString,  // dayparts->days_of_week
      mojom::DBBindColumnType::kInt,     // dayparts->start_minute
      mojom::DBBindColumnType::kInt,     // dayparts->end_minute
      mojom::DBBindColumnType::
          kString,  // creative_new_tab_page_ad_wallpapers->image_url
      mojom::DBBindColumnType::
          kInt,  // creative_new_tab_page_ad_wallpapers->focal_point->x
      mojom::DBBindColumnType::
          kInt  // creative_new_tab_page_ad_wallpapers->focal_point->y
  };
}

size_t BindColumns(mojom::DBStatementInfo* mojom_statement,
                   const CreativeNewTabPageAdList& creative_ads) {
  CHECK(mojom_statement);
  CHECK(!creative_ads.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindColumnString(mojom_statement, index++,
                     creative_ad.creative_instance_id);
    BindColumnString(mojom_statement, index++, creative_ad.creative_set_id);
    BindColumnString(mojom_statement, index++, creative_ad.campaign_id);
    BindColumnString(mojom_statement, index++, creative_ad.company_name);
    BindColumnString(mojom_statement, index++, creative_ad.image_url.spec());
    BindColumnString(mojom_statement, index++, creative_ad.alt);

    ++row_count;
  }

  return row_count;
}

CreativeNewTabPageAdInfo FromMojomRow(const mojom::DBRowInfo* const mojom_row) {
  CHECK(mojom_row);

  CreativeNewTabPageAdInfo creative_ad;

  creative_ad.creative_instance_id = ColumnString(mojom_row, 0);
  creative_ad.creative_set_id = ColumnString(mojom_row, 1);
  creative_ad.campaign_id = ColumnString(mojom_row, 2);
  creative_ad.start_at = ToTimeFromChromeTimestamp(ColumnInt64(mojom_row, 3));
  creative_ad.end_at = ToTimeFromChromeTimestamp(ColumnInt64(mojom_row, 4));
  creative_ad.daily_cap = ColumnInt(mojom_row, 5);
  creative_ad.advertiser_id = ColumnString(mojom_row, 6);
  creative_ad.priority = ColumnInt(mojom_row, 7);
  creative_ad.per_day = ColumnInt(mojom_row, 8);
  creative_ad.per_week = ColumnInt(mojom_row, 9);
  creative_ad.per_month = ColumnInt(mojom_row, 10);
  creative_ad.total_max = ColumnInt(mojom_row, 11);
  creative_ad.value = ColumnDouble(mojom_row, 12);
  creative_ad.split_test_group = ColumnString(mojom_row, 13);
  creative_ad.segment = ColumnString(mojom_row, 14);
  creative_ad.geo_targets.insert(ColumnString(mojom_row, 15));
  creative_ad.target_url = GURL(ColumnString(mojom_row, 16));
  creative_ad.company_name = ColumnString(mojom_row, 17);
  creative_ad.image_url = GURL(ColumnString(mojom_row, 18));
  creative_ad.alt = ColumnString(mojom_row, 19);
  creative_ad.pass_through_rate = ColumnDouble(mojom_row, 20);

  CreativeDaypartInfo daypart;
  daypart.days_of_week = ColumnString(mojom_row, 21);
  daypart.start_minute = ColumnInt(mojom_row, 22);
  daypart.end_minute = ColumnInt(mojom_row, 23);
  creative_ad.dayparts.push_back(daypart);

  CreativeNewTabPageAdWallpaperInfo wallpaper;
  wallpaper.image_url = GURL(ColumnString(mojom_row, 24));
  wallpaper.focal_point.x = ColumnInt(mojom_row, 25);
  wallpaper.focal_point.y = ColumnInt(mojom_row, 26);
  creative_ad.wallpapers.push_back(wallpaper);

  return creative_ad;
}

CreativeNewTabPageAdList GetCreativeAdsFromResponse(
    mojom::DBStatementResultInfoPtr mojom_statement_result) {
  CHECK(mojom_statement_result);
  CHECK(mojom_statement_result->rows_union);

  CreativeNewTabPageAdMap creative_ads;

  for (const auto& mojom_row : mojom_statement_result->rows_union->get_rows()) {
    const CreativeNewTabPageAdInfo creative_ad = FromMojomRow(&*mojom_row);

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
    mojom::DBStatementResultInfoPtr mojom_statement_result) {
  if (!mojom_statement_result ||
      mojom_statement_result->result_code !=
          mojom::DBStatementResultInfo::ResultCode::kSuccess) {
    BLOG(0, "Failed to get creative new tab page ad");

    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ads=*/{});
  }

  const CreativeNewTabPageAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(mojom_statement_result));

  if (creative_ads.size() != 1) {
    BLOG(0, "Failed to get creative new tab page ad");

    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ads=*/{});
  }

  const CreativeNewTabPageAdInfo& creative_ad = creative_ads.front();

  std::move(callback).Run(/*success=*/true, creative_instance_id, creative_ad);
}

void GetForSegmentsCallback(
    const SegmentList& segments,
    GetCreativeNewTabPageAdsCallback callback,
    mojom::DBStatementResultInfoPtr mojom_statement_result) {
  if (!mojom_statement_result ||
      mojom_statement_result->result_code !=
          mojom::DBStatementResultInfo::ResultCode::kSuccess) {
    BLOG(0, "Failed to get creative new tab page ads");

    return std::move(callback).Run(/*success=*/false, segments,
                                   /*creative_ads=*/{});
  }

  const CreativeNewTabPageAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(mojom_statement_result));

  std::move(callback).Run(/*success=*/true, segments, creative_ads);
}

void GetAllCallback(GetCreativeNewTabPageAdsCallback callback,
                    mojom::DBStatementResultInfoPtr mojom_statement_result) {
  if (!mojom_statement_result ||
      mojom_statement_result->result_code !=
          mojom::DBStatementResultInfo::ResultCode::kSuccess) {
    BLOG(0, "Failed to get all creative new tab page ads");

    return std::move(callback).Run(/*success=*/false, /*segments=*/{},
                                   /*creative_ads=*/{});
  }

  const CreativeNewTabPageAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(mojom_statement_result));

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

  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();

  const std::vector<CreativeNewTabPageAdList> batches =
      SplitVector(creative_ads, batch_size_);

  for (const auto& batch : batches) {
    Insert(&*mojom_transaction, batch);

    const CreativeAdList creative_ads_batch(batch.cbegin(), batch.cend());
    campaigns_database_table_.Insert(&*mojom_transaction, creative_ads_batch);
    creative_ads_database_table_.Insert(&*mojom_transaction,
                                        creative_ads_batch);
    creative_new_tab_page_ad_wallpapers_database_table_.Insert(
        &*mojom_transaction, batch);
    dayparts_database_table_.Insert(&*mojom_transaction, creative_ads_batch);
    deposits_database_table_.Insert(&*mojom_transaction, creative_ads_batch);
    geo_targets_database_table_.Insert(&*mojom_transaction, creative_ads_batch);
    segments_database_table_.Insert(&*mojom_transaction, creative_ads_batch);
  }

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void CreativeNewTabPageAds::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();

  DeleteTable(&*mojom_transaction, GetTableName());

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void CreativeNewTabPageAds::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativeNewTabPageAdCallback callback) const {
  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ads=*/{});
  }

  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kStep;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
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
            creative_ads.split_test_group,
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
  BindColumnTypes(&*mojom_statement);
  mojom_transaction->statements.push_back(std::move(mojom_statement));

  RunDBTransaction(std::move(mojom_transaction),
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

  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kStep;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
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
            creative_ads.split_test_group,
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
       BuildBindColumnPlaceholder(/*column_count=*/segments.size()),
       base::NumberToString(ToChromeTimestampFromTime(base::Time::Now()))},
      nullptr);
  BindColumnTypes(&*mojom_statement);

  int index = 0;
  for (const auto& segment : segments) {
    BindColumnString(&*mojom_statement, index++, segment);
  }

  mojom_transaction->statements.push_back(std::move(mojom_statement));

  RunDBTransaction(
      std::move(mojom_transaction),
      base::BindOnce(&GetForSegmentsCallback, segments, std::move(callback)));
}

void CreativeNewTabPageAds::GetForActiveCampaigns(
    GetCreativeNewTabPageAdsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kStep;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
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
            creative_ads.split_test_group,
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
  BindColumnTypes(&*mojom_statement);
  mojom_transaction->statements.push_back(std::move(mojom_statement));

  RunDBTransaction(std::move(mojom_transaction),
                   base::BindOnce(&GetAllCallback, std::move(callback)));
}

std::string CreativeNewTabPageAds::GetTableName() const {
  return kTableName;
}

void CreativeNewTabPageAds::Create(
    mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  Execute(mojom_transaction, R"(
      CREATE TABLE creative_new_tab_page_ads (
        creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        creative_set_id TEXT NOT NULL,
        campaign_id TEXT NOT NULL,
        company_name TEXT NOT NULL,
        image_url TEXT NOT NULL,
        alt TEXT NOT NULL
      );)");
}

void CreativeNewTabPageAds::Migrate(mojom::DBTransactionInfo* mojom_transaction,
                                    const int to_version) {
  CHECK(mojom_transaction);

  switch (to_version) {
    case 43: {
      MigrateToV43(mojom_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeNewTabPageAds::MigrateToV43(
    mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // We can safely recreate the table because it will be repopulated after
  // downloading the catalog.
  DropTable(mojom_transaction, GetTableName());
  Create(mojom_transaction);
}

void CreativeNewTabPageAds::Insert(
    mojom::DBTransactionInfo* mojom_transaction,
    const CreativeNewTabPageAdList& creative_ads) {
  CHECK(mojom_transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type = mojom::DBStatementInfo::OperationType::kRun;
  mojom_statement->sql = BuildInsertSql(&*mojom_statement, creative_ads);
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

std::string CreativeNewTabPageAds::BuildInsertSql(
    mojom::DBStatementInfo* mojom_statement,
    const CreativeNewTabPageAdList& creative_ads) const {
  CHECK(mojom_statement);
  CHECK(!creative_ads.empty());

  const size_t row_count = BindColumns(mojom_statement, creative_ads);

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
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/6, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
