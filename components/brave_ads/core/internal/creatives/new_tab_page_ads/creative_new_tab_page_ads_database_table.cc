/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/algorithm/split_vector_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/internal/content_settings/content_settings_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_util.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/condition_matcher_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-data-view.h"
#include "url/gurl.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "creative_new_tab_page_ads";

constexpr int kDefaultBatchSize = 50;

void BindColumnTypes(const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kString,  // creative_instance_id
      mojom::DBBindColumnType::kString,  // creative_set_id
      mojom::DBBindColumnType::kString,  // campaign_id
      mojom::DBBindColumnType::kString,  // metric_type
      mojom::DBBindColumnType::kTime,    // start_at
      mojom::DBBindColumnType::kTime,    // end_at
      mojom::DBBindColumnType::kInt,     // daily_cap
      mojom::DBBindColumnType::kString,  // advertiser_id
      mojom::DBBindColumnType::kInt,     // priority
      mojom::DBBindColumnType::kInt,     // per_day
      mojom::DBBindColumnType::kInt,     // per_week
      mojom::DBBindColumnType::kInt,     // per_month
      mojom::DBBindColumnType::kInt,     // total_max
      mojom::DBBindColumnType::kDouble,  // value
      mojom::DBBindColumnType::kString,  // split_test_group
      mojom::DBBindColumnType::kString,  // condition_matchers
      mojom::DBBindColumnType::kString,  // segment
      mojom::DBBindColumnType::kString,  // geo_target
      mojom::DBBindColumnType::kString,  // target_url
      mojom::DBBindColumnType::kString,  // type
      mojom::DBBindColumnType::kString,  // company_name
      mojom::DBBindColumnType::kString,  // alt
      mojom::DBBindColumnType::kDouble,  // ptr
      mojom::DBBindColumnType::kString,  // dayparts->days_of_week
      mojom::DBBindColumnType::kInt,     // dayparts->start_minute
      mojom::DBBindColumnType::kInt      // dayparts->end_minute
  };
}

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const CreativeNewTabPageAdList& creative_ads) {
  CHECK(mojom_db_action);
  CHECK(!creative_ads.empty());

  size_t row_count = 0;

  int32_t index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindColumnString(mojom_db_action, index++,
                     creative_ad.creative_instance_id);
    BindColumnString(mojom_db_action, index++, creative_ad.creative_set_id);
    BindColumnString(mojom_db_action, index++, creative_ad.campaign_id);
    BindColumnString(mojom_db_action, index++,
                     ToString(creative_ad.wallpaper_type));
    BindColumnString(mojom_db_action, index++, creative_ad.company_name);
    BindColumnString(mojom_db_action, index++, creative_ad.alt);

    ++row_count;
  }

  return row_count;
}

CreativeNewTabPageAdInfo FromMojomRow(const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  CreativeNewTabPageAdInfo creative_ad;

  creative_ad.creative_instance_id = ColumnString(mojom_db_row, 0);
  creative_ad.creative_set_id = ColumnString(mojom_db_row, 1);
  creative_ad.campaign_id = ColumnString(mojom_db_row, 2);
  creative_ad.metric_type =
      ToMojomNewTabPageAdMetricType(ColumnString(mojom_db_row, 3))
          .value_or(mojom::NewTabPageAdMetricType::kUndefined);
  creative_ad.start_at = ColumnTime(mojom_db_row, 4);
  creative_ad.end_at = ColumnTime(mojom_db_row, 5);
  creative_ad.daily_cap = ColumnInt(mojom_db_row, 6);
  creative_ad.advertiser_id = ColumnString(mojom_db_row, 7);
  creative_ad.priority = ColumnInt(mojom_db_row, 8);
  creative_ad.per_day = ColumnInt(mojom_db_row, 9);
  creative_ad.per_week = ColumnInt(mojom_db_row, 10);
  creative_ad.per_month = ColumnInt(mojom_db_row, 11);
  creative_ad.total_max = ColumnInt(mojom_db_row, 12);
  creative_ad.value = ColumnDouble(mojom_db_row, 13);
  creative_ad.split_test_group = ColumnString(mojom_db_row, 14);
  creative_ad.condition_matchers =
      StringToConditionMatchers(ColumnString(mojom_db_row, 15));
  creative_ad.segment = ColumnString(mojom_db_row, 16);
  creative_ad.geo_targets.insert(ColumnString(mojom_db_row, 17));
  creative_ad.target_url = GURL(ColumnString(mojom_db_row, 18));
  creative_ad.wallpaper_type =
      ToCreativeNewTabPageAdWallpaperType(ColumnString(mojom_db_row, 19));
  creative_ad.company_name = ColumnString(mojom_db_row, 20);
  creative_ad.alt = ColumnString(mojom_db_row, 21);
  creative_ad.pass_through_rate = ColumnDouble(mojom_db_row, 22);

  CreativeDaypartInfo daypart;
  daypart.days_of_week = ColumnString(mojom_db_row, 23);
  daypart.start_minute = ColumnInt(mojom_db_row, 24);
  daypart.end_minute = ColumnInt(mojom_db_row, 25);
  creative_ad.dayparts.insert(daypart);

  return creative_ad;
}

CreativeNewTabPageAdList GetCreativeAdsFromResponse(
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  CHECK(mojom_db_transaction_result);
  CHECK(mojom_db_transaction_result->rows_union);

  std::map<std::string, CreativeNewTabPageAdInfo> creative_ads;

  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    const CreativeNewTabPageAdInfo creative_ad = FromMojomRow(mojom_db_row);

    std::string uuid = creative_ad.creative_instance_id + creative_ad.segment;
    const auto [iter, inserted] =
        creative_ads.emplace(std::move(uuid), creative_ad);
    if (!inserted) {
      iter->second.geo_targets.insert(creative_ad.geo_targets.cbegin(),
                                      creative_ad.geo_targets.cend());
      iter->second.dayparts.insert(creative_ad.dayparts.cbegin(),
                                   creative_ad.dayparts.cend());
    }
  }

  CreativeNewTabPageAdList normalized_creative_ads;
  normalized_creative_ads.reserve(creative_ads.size());
  for (auto& [_, creative_ad] : creative_ads) {
    normalized_creative_ads.push_back(std::move(creative_ad));
  }

  return normalized_creative_ads;
}

void GetForCreativeInstanceIdCallback(
    const std::string& creative_instance_id,
    GetCreativeNewTabPageAdCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (!IsTransactionSuccessful(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get creative new tab page ad");
    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ad=*/{});
  }

  const CreativeNewTabPageAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(mojom_db_transaction_result));

  if (creative_ads.size() != 1) {
    BLOG(0, "Failed to get creative new tab page ad");
    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ad=*/{});
  }

  const CreativeNewTabPageAdInfo& creative_ad = creative_ads.front();

  std::move(callback).Run(/*success=*/true, creative_instance_id, creative_ad);
}

void GetForSegmentsCallback(
    const SegmentList& segments,
    GetCreativeNewTabPageAdsCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (!IsTransactionSuccessful(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get creative new tab page ads");
    return std::move(callback).Run(/*success=*/false, segments,
                                   /*creative_ads=*/{});
  }

  CreativeNewTabPageAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(mojom_db_transaction_result));

  std::move(callback).Run(/*success=*/true, segments, std::move(creative_ads));
}

void GetForActiveCampaignsCallback(
    GetCreativeNewTabPageAdsCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (!IsTransactionSuccessful(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get all creative new tab page ads");
    return std::move(callback).Run(/*success=*/false, /*segments=*/{},
                                   /*creative_ads=*/{});
  }

  CreativeNewTabPageAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(mojom_db_transaction_result));

  const SegmentList segments = GetSegments(creative_ads);

  std::move(callback).Run(/*success=*/true, segments, std::move(creative_ads));
}

std::string IsRichMediaAllowed() {
  return IsJavaScriptAllowed() ? "1" : "0";
}

}  // namespace

CreativeNewTabPageAds::CreativeNewTabPageAds()
    : batch_size_(kDefaultBatchSize) {}

CreativeNewTabPageAds::~CreativeNewTabPageAds() = default;

void CreativeNewTabPageAds::Save(const CreativeNewTabPageAdList& creative_ads,
                                 ResultCallback callback) {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  // Replace existing creative new tab page ads.

  database::Execute(mojom_db_transaction, R"(
      DELETE FROM
        campaigns
      WHERE
        id IN (
          SELECT
            DISTINCT campaign_id
          FROM
            creative_new_tab_page_ads
        ))");

  database::Execute(mojom_db_transaction, R"(
      DELETE FROM
        geo_targets
      WHERE
        campaign_id IN (
          SELECT
            DISTINCT campaign_id
          FROM
            creative_new_tab_page_ads
        ))");

  database::Execute(mojom_db_transaction, R"(
      DELETE FROM
        dayparts
      WHERE
        campaign_id IN (
          SELECT
            DISTINCT campaign_id
          FROM
            creative_new_tab_page_ads
        ))");

  database::Execute(mojom_db_transaction, R"(
      DELETE FROM
        segments
      WHERE
        creative_set_id IN (
          SELECT
            DISTINCT creative_set_id
          FROM
            creative_new_tab_page_ads
        ))");

  database::Execute(mojom_db_transaction, R"(
      DELETE FROM
        creative_ads
      WHERE
        creative_instance_id IN (
          SELECT
            DISTINCT creative_instance_id
          FROM
            creative_new_tab_page_ads
        ))");

  database::Execute(mojom_db_transaction, R"(
      DELETE FROM
        creative_new_tab_page_ads)");

  if (!creative_ads.empty()) {
    campaigns_database_table_.Insert(
        mojom_db_transaction,
        CreativeAdList(creative_ads.cbegin(), creative_ads.cend()));

    const std::vector<CreativeNewTabPageAdList> batches =
        SplitVector(DeduplicateCreativeAds(creative_ads), batch_size_);
    for (const auto& batch : batches) {
      Insert(mojom_db_transaction, batch);
    }
  }

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void CreativeNewTabPageAds::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativeNewTabPageAdCallback callback) const {
  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ad=*/{});
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteQueryWithBindings;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_new_tab_page_ad.creative_instance_id,
            creative_new_tab_page_ad.creative_set_id,
            creative_new_tab_page_ad.campaign_id,
            campaigns.metric_type,
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
            creative_ads.condition_matchers,
            segments.segment,
            geo_targets.geo_target,
            creative_ads.target_url,
            creative_new_tab_page_ad.type,
            creative_new_tab_page_ad.company_name,
            creative_new_tab_page_ad.alt,
            campaigns.ptr,
            dayparts.days_of_week,
            dayparts.start_minute,
            dayparts.end_minute
          FROM
            $1 AS creative_new_tab_page_ad
            INNER JOIN campaigns ON campaigns.id = creative_new_tab_page_ad.campaign_id
            INNER JOIN creative_ads ON creative_ads.creative_instance_id = creative_new_tab_page_ad.creative_instance_id
            INNER JOIN dayparts ON dayparts.campaign_id = creative_new_tab_page_ad.campaign_id
            INNER JOIN geo_targets ON geo_targets.campaign_id = creative_new_tab_page_ad.campaign_id
            INNER JOIN segments ON segments.creative_set_id = creative_new_tab_page_ad.creative_set_id
          WHERE
            creative_new_tab_page_ad.creative_instance_id = '$2'
            AND ($3 OR creative_new_tab_page_ad.type != 'richMedia'))",
      {GetTableName(), creative_instance_id, IsRichMediaAllowed()}, nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
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

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteQueryWithBindings;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_new_tab_page_ad.creative_instance_id,
            creative_new_tab_page_ad.creative_set_id,
            creative_new_tab_page_ad.campaign_id,
            campaigns.metric_type,
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
            creative_ads.condition_matchers,
            segments.segment,
            geo_targets.geo_target,
            creative_ads.target_url,
            creative_new_tab_page_ad.type,
            creative_new_tab_page_ad.company_name,
            creative_new_tab_page_ad.alt,
            campaigns.ptr,
            dayparts.days_of_week,
            dayparts.start_minute,
            dayparts.end_minute
          FROM
            $1 AS creative_new_tab_page_ad
            INNER JOIN campaigns ON campaigns.id = creative_new_tab_page_ad.campaign_id
            INNER JOIN creative_ads ON creative_ads.creative_instance_id = creative_new_tab_page_ad.creative_instance_id
            INNER JOIN dayparts ON dayparts.campaign_id = creative_new_tab_page_ad.campaign_id
            INNER JOIN geo_targets ON geo_targets.campaign_id = creative_new_tab_page_ad.campaign_id
            INNER JOIN segments ON segments.creative_set_id = creative_new_tab_page_ad.creative_set_id
          WHERE
            segments.segment IN $2
            AND $3 BETWEEN campaigns.start_at AND campaigns.end_at
            AND ($4 OR creative_new_tab_page_ad.type != 'richMedia'))",
      {GetTableName(),
       BuildBindColumnPlaceholder(/*column_count=*/segments.size()),
       TimeToSqlValueAsString(base::Time::Now()), IsRichMediaAllowed()},
      nullptr);
  BindColumnTypes(mojom_db_action);

  int32_t index = 0;
  for (const auto& segment : segments) {
    BindColumnString(mojom_db_action, index++, segment);
  }

  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunTransaction(
      FROM_HERE, std::move(mojom_db_transaction),
      base::BindOnce(&GetForSegmentsCallback, segments, std::move(callback)));
}

void CreativeNewTabPageAds::GetForActiveCampaigns(
    GetCreativeNewTabPageAdsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteQueryWithBindings;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_new_tab_page_ad.creative_instance_id,
            creative_new_tab_page_ad.creative_set_id,
            creative_new_tab_page_ad.campaign_id,
            campaigns.metric_type,
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
            creative_ads.condition_matchers,
            segments.segment,
            geo_targets.geo_target,
            creative_ads.target_url,
            creative_new_tab_page_ad.type,
            creative_new_tab_page_ad.company_name,
            creative_new_tab_page_ad.alt,
            campaigns.ptr,
            dayparts.days_of_week,
            dayparts.start_minute,
            dayparts.end_minute
          FROM
            $1 AS creative_new_tab_page_ad
            INNER JOIN campaigns ON campaigns.id = creative_new_tab_page_ad.campaign_id
            INNER JOIN creative_ads ON creative_ads.creative_instance_id = creative_new_tab_page_ad.creative_instance_id
            INNER JOIN dayparts ON dayparts.campaign_id = creative_new_tab_page_ad.campaign_id
            INNER JOIN geo_targets ON geo_targets.campaign_id = creative_new_tab_page_ad.campaign_id
            INNER JOIN segments ON segments.creative_set_id = creative_new_tab_page_ad.creative_set_id
          WHERE
            $2 BETWEEN campaigns.start_at AND campaigns.end_at
            AND ($3 OR creative_new_tab_page_ad.type != 'richMedia'))",
      {GetTableName(), TimeToSqlValueAsString(base::Time::Now()),
       IsRichMediaAllowed()},
      nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunTransaction(
      FROM_HERE, std::move(mojom_db_transaction),
      base::BindOnce(&GetForActiveCampaignsCallback, std::move(callback)));
}

std::string CreativeNewTabPageAds::GetTableName() const {
  return kTableName;
}

void CreativeNewTabPageAds::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE creative_new_tab_page_ads (
        creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        creative_set_id TEXT NOT NULL,
        campaign_id TEXT NOT NULL,
        type TEXT NOT NULL,
        company_name TEXT NOT NULL,
        alt TEXT NOT NULL
      ))");
}

void CreativeNewTabPageAds::Migrate(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 48: {
      MigrateToV48(mojom_db_transaction);
      break;
    }

    case 49: {
      MigrateToV49(mojom_db_transaction);
      break;
    }

    default: {
      // No migration needed.
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeNewTabPageAds::MigrateToV48(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Wallpapers table has been deprecated.
  DropTable(mojom_db_transaction, "creative_new_tab_page_ad_wallpapers");

  // It is safe to recreate the table because it will be repopulated after
  // downloading the component resource post-migration. However, after this
  // migration, we should not drop the table as it is needed to maintain
  // relationships with other tables.
  DropTable(mojom_db_transaction, "creative_new_tab_page_ads");

  Execute(mojom_db_transaction, R"(
      CREATE TABLE creative_new_tab_page_ads (
        creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        creative_set_id TEXT NOT NULL,
        campaign_id TEXT NOT NULL,
        company_name TEXT NOT NULL,
        alt TEXT NOT NULL
      );)");
}

void CreativeNewTabPageAds::MigrateToV49(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Create a temporary table:
  //   - with a new `type` column constraint. The default value for existing
  //     rows is 'image', which will be corrected when the new tab page ads are
  //     updated.
  Execute(mojom_db_transaction, R"(
      CREATE TABLE creative_new_tab_page_ads_temp (
        creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        creative_set_id TEXT NOT NULL,
        campaign_id TEXT NOT NULL,
        type TEXT NOT NULL DEFAULT 'image',
        company_name TEXT NOT NULL,
        alt TEXT NOT NULL
      ))");

  // Copy legacy columns to the temporary table, drop the legacy table and
  // rename the temporary table.
  const std::vector<std::string> columns = {"creative_instance_id",
                                            "creative_set_id", "campaign_id",
                                            "company_name", "alt"};

  CopyTableColumns(mojom_db_transaction, "creative_new_tab_page_ads",
                   "creative_new_tab_page_ads_temp", columns,
                   /*should_drop=*/true);

  RenameTable(mojom_db_transaction, "creative_new_tab_page_ads_temp",
              "creative_new_tab_page_ads");
}

void CreativeNewTabPageAds::Insert(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    const CreativeNewTabPageAdList& creative_ads) {
  CHECK(mojom_db_transaction);

  if (creative_ads.empty()) {
    return;
  }

  creative_ads_database_table_.Insert(
      mojom_db_transaction,
      CreativeAdList(creative_ads.cbegin(), creative_ads.cend()));

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, creative_ads);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

std::string CreativeNewTabPageAds::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const CreativeNewTabPageAdList& creative_ads) const {
  CHECK(mojom_db_action);
  CHECK(!creative_ads.empty());

  const size_t row_count = BindColumns(mojom_db_action, creative_ads);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_instance_id,
            creative_set_id,
            campaign_id,
            type,
            company_name,
            alt
          ) VALUES $2)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/6, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
