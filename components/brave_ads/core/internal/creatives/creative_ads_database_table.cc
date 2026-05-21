/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table.h"

#include <cstddef>
#include <map>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/condition_matchers_database_table_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

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
      mojom::DBBindColumnType::kString,  // condition_matchers
      mojom::DBBindColumnType::kString   // target_url
  };
}

CreativeAdList GetCreativeAdsFromResponse(
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  CHECK(mojom_db_transaction_result);
  CHECK(mojom_db_transaction_result->rows_union);

  std::map<std::string, CreativeAdInfo> creative_ads;

  for (const auto& mojom_db_row :
       mojom_db_transaction_result->rows_union->get_rows()) {
    CreativeAdInfo creative_ad = CreativeAdFromMojomRow(mojom_db_row);

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
            condition_matchers,
            target_url
          FROM
            $1
          WHERE
            creative_instance_id = '$2')",
      {kTableName, creative_instance_id}, nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 base::BindOnce(&GetForCreativeInstanceIdCallback,
                                creative_instance_id, std::move(callback)));
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

    case 54: {
      MigrateToV54(mojom_db_transaction);
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

void CreativeAds::MigrateToV54(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // `split_test_group` has been removed.
  Execute(mojom_db_transaction,
          "ALTER TABLE creative_ads DROP COLUMN split_test_group");
}

}  // namespace brave_ads::database::table
