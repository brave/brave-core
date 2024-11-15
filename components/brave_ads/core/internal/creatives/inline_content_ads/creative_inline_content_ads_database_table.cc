/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"

#include <cstddef>
#include <map>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
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
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "url/gurl.h"

namespace brave_ads::database::table {

using CreativeInlineContentAdMap =
    std::map</*creative_ad_uuid*/ std::string, CreativeInlineContentAdInfo>;

namespace {

constexpr char kTableName[] = "creative_inline_content_ads";

constexpr int kDefaultBatchSize = 50;

void BindColumnTypes(const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kString,  // creative_instance_id
      mojom::DBBindColumnType::kString,  // creative_set_id
      mojom::DBBindColumnType::kString,  // campaign_id
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
      mojom::DBBindColumnType::kString,  // segment
      mojom::DBBindColumnType::kString,  // geo_target
      mojom::DBBindColumnType::kString,  // target_url
      mojom::DBBindColumnType::kString,  // title
      mojom::DBBindColumnType::kString,  // description
      mojom::DBBindColumnType::kString,  // image_url
      mojom::DBBindColumnType::kString,  // dimensions
      mojom::DBBindColumnType::kString,  // cta_text
      mojom::DBBindColumnType::kDouble,  // ptr
      mojom::DBBindColumnType::kString,  // dayparts->days_of_week
      mojom::DBBindColumnType::kInt,     // dayparts->start_minute
      mojom::DBBindColumnType::kInt      // dayparts->end_minute
  };
}

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const CreativeInlineContentAdList& creative_ads) {
  CHECK(mojom_db_action);
  CHECK(!creative_ads.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindColumnString(mojom_db_action, index++,
                     creative_ad.creative_instance_id);
    BindColumnString(mojom_db_action, index++, creative_ad.creative_set_id);
    BindColumnString(mojom_db_action, index++, creative_ad.campaign_id);
    BindColumnString(mojom_db_action, index++, creative_ad.title);
    BindColumnString(mojom_db_action, index++, creative_ad.description);
    BindColumnString(mojom_db_action, index++, creative_ad.image_url.spec());
    BindColumnString(mojom_db_action, index++, creative_ad.dimensions);
    BindColumnString(mojom_db_action, index++, creative_ad.cta_text);

    ++row_count;
  }

  return row_count;
}

CreativeInlineContentAdInfo FromMojomRow(
    const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  CreativeInlineContentAdInfo creative_ad;

  creative_ad.creative_instance_id = ColumnString(mojom_db_row, 0);
  creative_ad.creative_set_id = ColumnString(mojom_db_row, 1);
  creative_ad.campaign_id = ColumnString(mojom_db_row, 2);
  creative_ad.start_at = ColumnTime(mojom_db_row, 3);
  creative_ad.end_at = ColumnTime(mojom_db_row, 4);
  creative_ad.daily_cap = ColumnInt(mojom_db_row, 5);
  creative_ad.advertiser_id = ColumnString(mojom_db_row, 6);
  creative_ad.priority = ColumnInt(mojom_db_row, 7);
  creative_ad.per_day = ColumnInt(mojom_db_row, 8);
  creative_ad.per_week = ColumnInt(mojom_db_row, 9);
  creative_ad.per_month = ColumnInt(mojom_db_row, 10);
  creative_ad.total_max = ColumnInt(mojom_db_row, 11);
  creative_ad.value = ColumnDouble(mojom_db_row, 12);
  creative_ad.split_test_group = ColumnString(mojom_db_row, 13);
  creative_ad.segment = ColumnString(mojom_db_row, 14);
  creative_ad.geo_targets.insert(ColumnString(mojom_db_row, 15));
  creative_ad.target_url = GURL(ColumnString(mojom_db_row, 16));
  creative_ad.title = ColumnString(mojom_db_row, 17);
  creative_ad.description = ColumnString(mojom_db_row, 18);
  creative_ad.image_url = GURL(ColumnString(mojom_db_row, 19));
  creative_ad.dimensions = ColumnString(mojom_db_row, 20);
  creative_ad.cta_text = ColumnString(mojom_db_row, 21);
  creative_ad.pass_through_rate = ColumnDouble(mojom_db_row, 22);

  CreativeDaypartInfo daypart;
  daypart.days_of_week = ColumnString(mojom_db_row, 23);
  daypart.start_minute = ColumnInt(mojom_db_row, 24);
  daypart.end_minute = ColumnInt(mojom_db_row, 25);
  creative_ad.dayparts.push_back(daypart);

  return creative_ad;
}

CreativeInlineContentAdList GetCreativeAdsFromResponse(
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  CHECK(mojom_db_transaction_result);
  CHECK(mojom_db_transaction_result->rows_union);

  CreativeInlineContentAdMap creative_ads;

  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    const CreativeInlineContentAdInfo creative_ad = FromMojomRow(mojom_db_row);

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
  }

  CreativeInlineContentAdList normalized_creative_ads;
  for (const auto& [_, creative_ad] : creative_ads) {
    normalized_creative_ads.push_back(creative_ad);
  }

  return normalized_creative_ads;
}

void GetForCreativeInstanceIdCallback(
    const std::string& creative_instance_id,
    GetCreativeInlineContentAdCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (IsError(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get creative inline content ad");

    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ad=*/{});
  }

  const CreativeInlineContentAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(mojom_db_transaction_result));

  if (creative_ads.size() != 1) {
    BLOG(0, "Failed to get creative inline content ad");

    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ad=*/{});
  }

  const CreativeInlineContentAdInfo& creative_ad = creative_ads.front();

  std::move(callback).Run(/*success=*/true, creative_instance_id, creative_ad);
}

void GetForSegmentsAndDimensionsCallback(
    const SegmentList& segments,
    GetCreativeInlineContentAdsCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (IsError(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get creative inline content ads");

    return std::move(callback).Run(/*success=*/false, segments,
                                   /*creative_ad=*/{});
  }

  const CreativeInlineContentAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(mojom_db_transaction_result));

  std::move(callback).Run(/*success=*/true, segments, creative_ads);
}

void GetForDimensionsCallback(
    GetCreativeInlineContentAdsForDimensionsCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (IsError(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get creative inline content ads");

    return std::move(callback).Run(/*success=*/false, /*creative_ad=*/{});
  }

  const CreativeInlineContentAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(mojom_db_transaction_result));

  std::move(callback).Run(/*success=*/true, creative_ads);
}

void GetAllCallback(
    GetCreativeInlineContentAdsCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (IsError(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get all creative inline content ads");

    return std::move(callback).Run(/*success=*/false, /*segments=*/{},
                                   /*creative_ads=*/{});
  }

  const CreativeInlineContentAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(mojom_db_transaction_result));

  const SegmentList segments = GetSegments(creative_ads);

  std::move(callback).Run(/*success=*/true, segments, creative_ads);
}

}  // namespace

CreativeInlineContentAds::CreativeInlineContentAds()
    : batch_size_(kDefaultBatchSize) {}

CreativeInlineContentAds::~CreativeInlineContentAds() = default;

void CreativeInlineContentAds::Save(
    const CreativeInlineContentAdList& creative_ads,
    ResultCallback callback) {
  if (creative_ads.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  const std::vector<CreativeInlineContentAdList> batches =
      SplitVector(creative_ads, batch_size_);

  for (const auto& batch : batches) {
    Insert(mojom_db_transaction, batch);

    const CreativeAdList creative_ads_batch(batch.cbegin(), batch.cend());
    campaigns_database_table_.Insert(mojom_db_transaction, creative_ads_batch);
    creative_ads_database_table_.Insert(mojom_db_transaction,
                                        creative_ads_batch);
    dayparts_database_table_.Insert(mojom_db_transaction, creative_ads_batch);
    deposits_database_table_.Insert(mojom_db_transaction, creative_ads_batch);
    geo_targets_database_table_.Insert(mojom_db_transaction,
                                       creative_ads_batch);
    segments_database_table_.Insert(mojom_db_transaction, creative_ads_batch);
  }

  RunDBTransaction(std::move(mojom_db_transaction), std::move(callback));
}

void CreativeInlineContentAds::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  DeleteTable(mojom_db_transaction, GetTableName());

  RunDBTransaction(std::move(mojom_db_transaction), std::move(callback));
}

void CreativeInlineContentAds::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativeInlineContentAdCallback callback) const {
  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ads=*/{});
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_inline_content_ad.creative_instance_id,
            creative_inline_content_ad.creative_set_id,
            creative_inline_content_ad.campaign_id,
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
            creative_inline_content_ad.title,
            creative_inline_content_ad.description,
            creative_inline_content_ad.image_url,
            creative_inline_content_ad.dimensions,
            creative_inline_content_ad.cta_text,
            campaigns.ptr,
            dayparts.days_of_week,
            dayparts.start_minute,
            dayparts.end_minute
          FROM
            $1 AS creative_inline_content_ad
            INNER JOIN campaigns ON campaigns.id = creative_inline_content_ad.campaign_id
            INNER JOIN creative_ads ON creative_ads.creative_instance_id = creative_inline_content_ad.creative_instance_id
            INNER JOIN dayparts ON dayparts.campaign_id = creative_inline_content_ad.campaign_id
            INNER JOIN geo_targets ON geo_targets.campaign_id = creative_inline_content_ad.campaign_id
            INNER JOIN segments ON segments.creative_set_id = creative_inline_content_ad.creative_set_id
          WHERE
            creative_inline_content_ad.creative_instance_id = '$2';)",
      {GetTableName(), creative_instance_id}, nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  GetAdsClient().RunDBTransaction(
      std::move(mojom_db_transaction),
      base::BindOnce(&GetForCreativeInstanceIdCallback, creative_instance_id,
                     std::move(callback)));
}

void CreativeInlineContentAds::GetForSegmentsAndDimensions(
    const SegmentList& segments,
    const std::string& dimensions,
    GetCreativeInlineContentAdsCallback callback) const {
  if (segments.empty() || dimensions.empty()) {
    return std::move(callback).Run(/*success=*/true, segments,
                                   /*creative_ads=*/{});
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_inline_content_ad.creative_instance_id,
            creative_inline_content_ad.creative_set_id,
            creative_inline_content_ad.campaign_id,
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
            creative_inline_content_ad.title,
            creative_inline_content_ad.description,
            creative_inline_content_ad.image_url,
            creative_inline_content_ad.dimensions,
            creative_inline_content_ad.cta_text,
            campaigns.ptr,
            dayparts.days_of_week,
            dayparts.start_minute,
            dayparts.end_minute
          FROM
            $1 AS creative_inline_content_ad
            INNER JOIN campaigns ON campaigns.id = creative_inline_content_ad.campaign_id
            INNER JOIN creative_ads ON creative_ads.creative_instance_id = creative_inline_content_ad.creative_instance_id
            INNER JOIN dayparts ON dayparts.campaign_id = creative_inline_content_ad.campaign_id
            INNER JOIN geo_targets ON geo_targets.campaign_id = creative_inline_content_ad.campaign_id
            INNER JOIN segments ON segments.creative_set_id = creative_inline_content_ad.creative_set_id
          WHERE
            segments.segment IN $2
            AND creative_inline_content_ad.dimensions = '$3'
            AND $4 BETWEEN campaigns.start_at AND campaigns.end_at;)",
      {GetTableName(),
       BuildBindColumnPlaceholder(/*column_count=*/segments.size()), dimensions,
       TimeToSqlValueAsString(base::Time::Now())},
      nullptr);
  BindColumnTypes(mojom_db_action);

  int index = 0;
  for (const auto& segment : segments) {
    BindColumnString(mojom_db_action, index++, segment);
  }

  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  GetAdsClient().RunDBTransaction(
      std::move(mojom_db_transaction),
      base::BindOnce(&GetForSegmentsAndDimensionsCallback, segments,
                     std::move(callback)));
}

void CreativeInlineContentAds::GetForDimensions(
    const std::string& dimensions,
    GetCreativeInlineContentAdsForDimensionsCallback callback) const {
  if (dimensions.empty()) {
    return std::move(callback).Run(/*success=*/true, /*creative_ads=*/{});
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_inline_content_ad.creative_instance_id,
            creative_inline_content_ad.creative_set_id,
            creative_inline_content_ad.campaign_id,
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
            creative_inline_content_ad.title,
            creative_inline_content_ad.description,
            creative_inline_content_ad.image_url,
            creative_inline_content_ad.dimensions,
            creative_inline_content_ad.cta_text,
            campaigns.ptr,
            dayparts.days_of_week,
            dayparts.start_minute,
            dayparts.end_minute
          FROM
            $1 AS creative_inline_content_ad
            INNER JOIN campaigns ON campaigns.id = creative_inline_content_ad.campaign_id
            INNER JOIN creative_ads ON creative_ads.creative_instance_id = creative_inline_content_ad.creative_instance_id
            INNER JOIN dayparts ON dayparts.campaign_id = creative_inline_content_ad.campaign_id
            INNER JOIN geo_targets ON geo_targets.campaign_id = creative_inline_content_ad.campaign_id
            INNER JOIN segments ON segments.creative_set_id = creative_inline_content_ad.creative_set_id
          WHERE
            creative_inline_content_ad.dimensions = '$2'
            AND $3 BETWEEN campaigns.start_at AND campaigns.end_at;)",
      {GetTableName(), dimensions, TimeToSqlValueAsString(base::Time::Now())},
      nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  GetAdsClient().RunDBTransaction(
      std::move(mojom_db_transaction),
      base::BindOnce(&GetForDimensionsCallback, std::move(callback)));
}

void CreativeInlineContentAds::GetForActiveCampaigns(
    GetCreativeInlineContentAdsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kStepStatement;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_inline_content_ad.creative_instance_id,
            creative_inline_content_ad.creative_set_id,
            creative_inline_content_ad.campaign_id,
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
            creative_inline_content_ad.title,
            creative_inline_content_ad.description,
            creative_inline_content_ad.image_url,
            creative_inline_content_ad.dimensions,
            creative_inline_content_ad.cta_text,
            campaigns.ptr,
            dayparts.days_of_week,
            dayparts.start_minute,
            dayparts.end_minute
          FROM
            $1 AS creative_inline_content_ad
            INNER JOIN campaigns ON campaigns.id = creative_inline_content_ad.campaign_id
            INNER JOIN creative_ads ON creative_ads.creative_instance_id = creative_inline_content_ad.creative_instance_id
            INNER JOIN dayparts ON dayparts.campaign_id = creative_inline_content_ad.campaign_id
            INNER JOIN geo_targets ON geo_targets.campaign_id = creative_inline_content_ad.campaign_id
            INNER JOIN segments ON segments.creative_set_id = creative_inline_content_ad.creative_set_id
          WHERE
            $2 BETWEEN campaigns.start_at AND campaigns.end_at;)",
      {GetTableName(), TimeToSqlValueAsString(base::Time::Now())}, nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  GetAdsClient().RunDBTransaction(
      std::move(mojom_db_transaction),
      base::BindOnce(&GetAllCallback, std::move(callback)));
}

std::string CreativeInlineContentAds::GetTableName() const {
  return kTableName;
}

void CreativeInlineContentAds::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE creative_inline_content_ads (
        creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        creative_set_id TEXT NOT NULL,
        campaign_id TEXT NOT NULL,
        title TEXT NOT NULL,
        description TEXT NOT NULL,
        image_url TEXT NOT NULL,
        dimensions TEXT NOT NULL,
        cta_text TEXT NOT NULL
      );)");
}

void CreativeInlineContentAds::Migrate(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    const int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 45: {
      MigrateToV45(mojom_db_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeInlineContentAds::MigrateToV45(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // We can safely recreate the table because it will be repopulated after
  // downloading the catalog.
  DropTable(mojom_db_transaction, GetTableName());
  Create(mojom_db_transaction);
}

void CreativeInlineContentAds::Insert(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    const CreativeInlineContentAdList& creative_ads) {
  CHECK(mojom_db_transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kRunStatement;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, creative_ads);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

std::string CreativeInlineContentAds::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const CreativeInlineContentAdList& creative_ads) const {
  CHECK(mojom_db_action);
  CHECK(!creative_ads.empty());

  const size_t row_count = BindColumns(mojom_db_action, creative_ads);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_instance_id,
            creative_set_id,
            campaign_id,
            title,
            description,
            image_url,
            dimensions,
            cta_text
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/8, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
