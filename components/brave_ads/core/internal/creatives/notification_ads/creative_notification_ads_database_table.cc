/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table.h"

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
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads::database::table {

using CreativeNotificationAdMap =
    std::map</*creative_ad_uuid*/ std::string, CreativeNotificationAdInfo>;

namespace {

constexpr char kTableName[] = "creative_ad_notifications";

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
      mojom::DBBindColumnType::kString,  // title
      mojom::DBBindColumnType::kString,  // body
      mojom::DBBindColumnType::kDouble,  // ptr
      mojom::DBBindColumnType::kString,  // dayparts->days_of_week
      mojom::DBBindColumnType::kInt,     // dayparts->start_minute
      mojom::DBBindColumnType::kInt      // dayparts->end_minute
  };
}

size_t BindColumns(mojom::DBStatementInfo* mojom_statement,
                   const CreativeNotificationAdList& creative_ads) {
  CHECK(mojom_statement);
  CHECK(!creative_ads.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindColumnString(mojom_statement, index++,
                     creative_ad.creative_instance_id);
    BindColumnString(mojom_statement, index++, creative_ad.creative_set_id);
    BindColumnString(mojom_statement, index++, creative_ad.campaign_id);
    BindColumnString(mojom_statement, index++, creative_ad.title);
    BindColumnString(mojom_statement, index++, creative_ad.body);

    ++row_count;
  }

  return row_count;
}

CreativeNotificationAdInfo FromMojomRow(
    const mojom::DBRowInfo* const mojom_row) {
  CHECK(mojom_row);

  CreativeNotificationAdInfo creative_ad;

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
  creative_ad.title = ColumnString(mojom_row, 17);
  creative_ad.body = ColumnString(mojom_row, 18);
  creative_ad.pass_through_rate = ColumnDouble(mojom_row, 19);

  CreativeDaypartInfo daypart;
  daypart.days_of_week = ColumnString(mojom_row, 20);
  daypart.start_minute = ColumnInt(mojom_row, 21);
  daypart.end_minute = ColumnInt(mojom_row, 22);
  creative_ad.dayparts.push_back(daypart);

  return creative_ad;
}

CreativeNotificationAdList GetCreativeAdsFromResponse(
    mojom::DBStatementResultInfoPtr mojom_statement_result) {
  CHECK(mojom_statement_result);
  CHECK(mojom_statement_result->rows_union);

  CreativeNotificationAdMap creative_ads;

  for (const auto& mojom_row : mojom_statement_result->rows_union->get_rows()) {
    const CreativeNotificationAdInfo creative_ad = FromMojomRow(&*mojom_row);

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

  CreativeNotificationAdList normalized_creative_ads;
  for (const auto& [_, creative_ad] : creative_ads) {
    normalized_creative_ads.push_back(creative_ad);
  }

  return normalized_creative_ads;
}

void GetForSegmentsCallback(
    const SegmentList& segments,
    GetCreativeNotificationAdsCallback callback,
    mojom::DBStatementResultInfoPtr mojom_statement_result) {
  if (!mojom_statement_result ||
      mojom_statement_result->result_code !=
          mojom::DBStatementResultInfo::ResultCode::kSuccess) {
    BLOG(0, "Failed to get creative notification ads");

    return std::move(callback).Run(/*success=*/false, segments,
                                   /*creative_ads=*/{});
  }

  const CreativeNotificationAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(mojom_statement_result));

  std::move(callback).Run(/*success=*/true, segments, creative_ads);
}

void GetAllCallback(GetCreativeNotificationAdsCallback callback,
                    mojom::DBStatementResultInfoPtr mojom_statement_result) {
  if (!mojom_statement_result ||
      mojom_statement_result->result_code !=
          mojom::DBStatementResultInfo::ResultCode::kSuccess) {
    BLOG(0, "Failed to get all creative notification ads");

    return std::move(callback).Run(/*success=*/false, /*segments=*/{},
                                   /*creative_ads=*/{});
  }

  const CreativeNotificationAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(mojom_statement_result));

  const SegmentList segments = GetSegments(creative_ads);

  std::move(callback).Run(/*success=*/true, segments, creative_ads);
}

}  // namespace

CreativeNotificationAds::CreativeNotificationAds()
    : batch_size_(kDefaultBatchSize) {}

CreativeNotificationAds::~CreativeNotificationAds() = default;

void CreativeNotificationAds::Save(
    const CreativeNotificationAdList& creative_ads,
    ResultCallback callback) {
  if (creative_ads.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();

  const std::vector<CreativeNotificationAdList> batches =
      SplitVector(creative_ads, batch_size_);

  for (const auto& batch : batches) {
    Insert(&*mojom_transaction, batch);

    const CreativeAdList creative_ads_batch(batch.cbegin(), batch.cend());
    campaigns_database_table_.Insert(&*mojom_transaction, creative_ads_batch);
    creative_ads_database_table_.Insert(&*mojom_transaction,
                                        creative_ads_batch);
    dayparts_database_table_.Insert(&*mojom_transaction, creative_ads_batch);
    deposits_database_table_.Insert(&*mojom_transaction, creative_ads_batch);
    geo_targets_database_table_.Insert(&*mojom_transaction, creative_ads_batch);
    segments_database_table_.Insert(&*mojom_transaction, creative_ads_batch);
  }

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void CreativeNotificationAds::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();

  DeleteTable(&*mojom_transaction, GetTableName());

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void CreativeNotificationAds::GetForSegments(
    const SegmentList& segments,
    GetCreativeNotificationAdsCallback callback) const {
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
            creative_notification_ad.creative_instance_id,
            creative_notification_ad.creative_set_id,
            creative_notification_ad.campaign_id,
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
            creative_notification_ad.title,
            creative_notification_ad.body,
            campaigns.ptr,
            dayparts.days_of_week,
            dayparts.start_minute,
            dayparts.end_minute
          FROM
            $1 AS creative_notification_ad
            INNER JOIN campaigns ON campaigns.id = creative_notification_ad.campaign_id
            INNER JOIN creative_ads ON creative_ads.creative_instance_id = creative_notification_ad.creative_instance_id
            INNER JOIN dayparts ON dayparts.campaign_id = creative_notification_ad.campaign_id
            INNER JOIN geo_targets ON geo_targets.campaign_id = creative_notification_ad.campaign_id
            INNER JOIN segments ON segments.creative_set_id = creative_notification_ad.creative_set_id
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
    BindColumnString(&*mojom_statement, index, segment);
    ++index;
  }

  mojom_transaction->statements.push_back(std::move(mojom_statement));

  RunDBTransaction(
      std::move(mojom_transaction),
      base::BindOnce(&GetForSegmentsCallback, segments, std::move(callback)));
}

void CreativeNotificationAds::GetForActiveCampaigns(
    GetCreativeNotificationAdsCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kStep;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_notification_ad.creative_instance_id,
            creative_notification_ad.creative_set_id,
            creative_notification_ad.campaign_id,
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
            creative_notification_ad.title,
            creative_notification_ad.body,
            campaigns.ptr,
            dayparts.days_of_week,
            dayparts.start_minute,
            dayparts.end_minute
          FROM
            $1 AS creative_notification_ad
            INNER JOIN campaigns ON campaigns.id = creative_notification_ad.campaign_id
            INNER JOIN creative_ads ON creative_ads.creative_instance_id = creative_notification_ad.creative_instance_id
            INNER JOIN dayparts ON dayparts.campaign_id = creative_notification_ad.campaign_id
            INNER JOIN geo_targets ON geo_targets.campaign_id = creative_notification_ad.campaign_id
            INNER JOIN segments ON segments.creative_set_id = creative_notification_ad.creative_set_id
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

std::string CreativeNotificationAds::GetTableName() const {
  return kTableName;
}

void CreativeNotificationAds::Create(
    mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kExecute;
  mojom_statement->sql =
      R"(
          CREATE TABLE creative_ad_notifications (
            creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
            creative_set_id TEXT NOT NULL,
            campaign_id TEXT NOT NULL,
            title TEXT NOT NULL,
            body TEXT NOT NULL
          );)";
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

void CreativeNotificationAds::Migrate(
    mojom::DBTransactionInfo* mojom_transaction,
    const int to_version) {
  CHECK(mojom_transaction);

  switch (to_version) {
    case 37: {
      MigrateToV37(mojom_transaction);
      break;
    }

    case 43: {
      MigrateToV43(mojom_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

// static
void CreativeNotificationAds::MigrateToV37(
    mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  DropTable(mojom_transaction, "embeddings");
  DropTable(mojom_transaction, "text_embedding_html_events");
}

void CreativeNotificationAds::MigrateToV43(
    mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // We can safely recreate the table because it will be repopulated after
  // downloading the catalog.
  DropTable(mojom_transaction, GetTableName());
  Create(mojom_transaction);
}

void CreativeNotificationAds::Insert(
    mojom::DBTransactionInfo* mojom_transaction,
    const CreativeNotificationAdList& creative_ads) {
  CHECK(mojom_transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type = mojom::DBStatementInfo::OperationType::kRun;
  mojom_statement->sql = BuildInsertSql(&*mojom_statement, creative_ads);
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

std::string CreativeNotificationAds::BuildInsertSql(
    mojom::DBStatementInfo* mojom_statement,
    const CreativeNotificationAdList& creative_ads) const {
  CHECK(mojom_statement);
  CHECK(!creative_ads.empty());

  const size_t row_count = BindColumns(mojom_statement, creative_ads);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_instance_id,
            creative_set_id,
            campaign_id,
            title,
            body
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/5, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
