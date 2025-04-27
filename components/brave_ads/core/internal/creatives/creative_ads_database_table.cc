/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "creative_ads";

void BindColumnTypes(const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kString,  // creative_instance_id
      mojom::DBBindColumnType::kString,  // creative_set_id
      mojom::DBBindColumnType::kInt,     // per_day
      mojom::DBBindColumnType::kInt,     // per_week
      mojom::DBBindColumnType::kInt,     // per_month
      mojom::DBBindColumnType::kInt,     // total_max
      mojom::DBBindColumnType::kDouble,  // value
      mojom::DBBindColumnType::kString,  // split_test_group
      mojom::DBBindColumnType::kString,  // condition_matchers
      mojom::DBBindColumnType::kString   // target_url
  };
}

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const CreativeAdList& creative_ads) {
  CHECK(mojom_db_action);
  CHECK(!creative_ads.empty());

  size_t row_count = 0;

  int32_t index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindColumnString(mojom_db_action, index++,
                     creative_ad.creative_instance_id);
    BindColumnString(mojom_db_action, index++, creative_ad.creative_set_id);
    BindColumnInt(mojom_db_action, index++, creative_ad.per_day);
    BindColumnInt(mojom_db_action, index++, creative_ad.per_week);
    BindColumnInt(mojom_db_action, index++, creative_ad.per_month);
    BindColumnInt(mojom_db_action, index++, creative_ad.total_max);
    BindColumnDouble(mojom_db_action, index++, creative_ad.value);
    BindColumnString(mojom_db_action, index++, creative_ad.split_test_group);
    BindColumnString(mojom_db_action, index++,
                     ConditionMatchersToString(creative_ad.condition_matchers));
    BindColumnString(mojom_db_action, index++, creative_ad.target_url.spec());

    ++row_count;
  }

  return row_count;
}

CreativeAdInfo FromMojomRow(const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  CreativeAdInfo creative_ad;

  creative_ad.creative_instance_id = ColumnString(mojom_db_row, 0);
  creative_ad.creative_set_id = ColumnString(mojom_db_row, 1);
  creative_ad.per_day = ColumnInt(mojom_db_row, 2);
  creative_ad.per_week = ColumnInt(mojom_db_row, 3);
  creative_ad.per_month = ColumnInt(mojom_db_row, 4);
  creative_ad.total_max = ColumnInt(mojom_db_row, 5);
  creative_ad.value = ColumnDouble(mojom_db_row, 6);
  creative_ad.split_test_group = ColumnString(mojom_db_row, 7);
  creative_ad.condition_matchers =
      StringToConditionMatchers(ColumnString(mojom_db_row, 8));
  creative_ad.target_url = GURL(ColumnString(mojom_db_row, 9));

  return creative_ad;
}

CreativeAdList GetCreativeAdsFromResponse(
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  CHECK(mojom_db_transaction_result);
  CHECK(mojom_db_transaction_result->rows_union);

  std::map<std::string, CreativeAdInfo> creative_ads;

  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    CreativeAdInfo creative_ad = FromMojomRow(mojom_db_row);

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

  CreativeAdList normalized_creative_ads;
  normalized_creative_ads.reserve(creative_ads.size());
  for (auto& [_, creative_ad] : creative_ads) {
    normalized_creative_ads.push_back(std::move(creative_ad));
  }

  return normalized_creative_ads;
}

void GetForCreativeInstanceIdCallback(
    const std::string& creative_instance_id,
    GetCreativeAdCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (!IsTransactionSuccessful(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get creative ad");
    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ad=*/{});
  }

  const CreativeAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(mojom_db_transaction_result));

  if (creative_ads.size() != 1) {
    BLOG(0, "Failed to get creative ad");
    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ad=*/{});
  }

  const CreativeAdInfo& creative_ad = creative_ads.front();

  std::move(callback).Run(/*success=*/true, creative_instance_id, creative_ad);
}

}  // namespace

void CreativeAds::Insert(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    const CreativeAdList& creative_ads) {
  CHECK(mojom_db_transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, creative_ads);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

void CreativeAds::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativeAdCallback callback) const {
  const CreativeAdInfo creative_ad;

  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   creative_ad);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteQueryWithBindings;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_instance_id,
            creative_set_id,
            per_day,
            per_week,
            per_month,
            total_max,
            value,
            split_test_group,
            target_url
          FROM
            $1
          WHERE
            creative_instance_id = '$2')",
      {GetTableName(), creative_instance_id}, nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 base::BindOnce(&GetForCreativeInstanceIdCallback,
                                creative_instance_id, std::move(callback)));
}

std::string CreativeAds::GetTableName() const {
  return kTableName;
}

void CreativeAds::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE creative_ads (
        creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        creative_set_id TEXT NOT NULL,
        per_day INTEGER NOT NULL DEFAULT 0,
        per_week INTEGER NOT NULL DEFAULT 0,
        per_month INTEGER NOT NULL DEFAULT 0,
        total_max INTEGER NOT NULL DEFAULT 0,
        value DOUBLE NOT NULL DEFAULT 0,
        split_test_group TEXT,
        condition_matchers TEXT NOT NULL,
        target_url TEXT NOT NULL
      ))");
}

void CreativeAds::Migrate(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 48: {
      MigrateToV48(mojom_db_transaction);
      break;
    }

    default: {
      // No migration needed.
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeAds::MigrateToV48(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // It is safe to recreate the table because it will be repopulated after
  // downloading the catalog post-migration. However, after this migration, we
  // should not drop the table as it will store catalog and non-catalog ad units
  // and maintain relationships with other tables.
  DropTable(mojom_db_transaction, "creative_ads");
  Create(mojom_db_transaction);
}

std::string CreativeAds::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const CreativeAdList& creative_ads) const {
  CHECK(mojom_db_action);
  CHECK(!creative_ads.empty());

  const size_t row_count = BindColumns(mojom_db_action, creative_ads);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_instance_id,
            creative_set_id,
            per_day,
            per_week,
            per_month,
            total_max,
            value,
            split_test_group,
            condition_matchers,
            target_url
          ) VALUES $2)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/10, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
