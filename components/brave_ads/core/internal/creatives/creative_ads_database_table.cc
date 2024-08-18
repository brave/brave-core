/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table.h"

#include <cstddef>
#include <map>
#include <utility>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "url/gurl.h"

namespace brave_ads::database::table {

using CreativeAdMap =
    std::map</*creative_ad_uuid*/ std::string, CreativeAdInfo>;

namespace {

constexpr char kTableName[] = "creative_ads";

void BindColumnTypes(mojom::DBStatementInfo* const mojom_statement) {
  CHECK(mojom_statement);

  mojom_statement->bind_column_types = {
      mojom::DBBindColumnType::kString,  // creative_instance_id
      mojom::DBBindColumnType::kString,  // creative_set_id
      mojom::DBBindColumnType::kInt,     // per_day
      mojom::DBBindColumnType::kInt,     // per_week
      mojom::DBBindColumnType::kInt,     // per_month
      mojom::DBBindColumnType::kInt,     // total_max
      mojom::DBBindColumnType::kDouble,  // value
      mojom::DBBindColumnType::kString,  // split_test_group
      mojom::DBBindColumnType::kString   // target_url
  };
}

size_t BindColumns(mojom::DBStatementInfo* mojom_statement,
                   const CreativeAdList& creative_ads) {
  CHECK(mojom_statement);
  CHECK(!creative_ads.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindColumnString(mojom_statement, index++,
                     creative_ad.creative_instance_id);
    BindColumnString(mojom_statement, index++, creative_ad.creative_set_id);
    BindColumnInt(mojom_statement, index++, creative_ad.per_day);
    BindColumnInt(mojom_statement, index++, creative_ad.per_week);
    BindColumnInt(mojom_statement, index++, creative_ad.per_month);
    BindColumnInt(mojom_statement, index++, creative_ad.total_max);
    BindColumnDouble(mojom_statement, index++, creative_ad.value);
    BindColumnString(mojom_statement, index++, creative_ad.split_test_group);
    BindColumnString(mojom_statement, index++, creative_ad.target_url.spec());

    ++row_count;
  }

  return row_count;
}

CreativeAdInfo FromMojomRow(const mojom::DBRowInfo* const mojom_row) {
  CHECK(mojom_row);

  CreativeAdInfo creative_ad;

  creative_ad.creative_instance_id = ColumnString(mojom_row, 0);
  creative_ad.creative_set_id = ColumnString(mojom_row, 1);
  creative_ad.per_day = ColumnInt(mojom_row, 2);
  creative_ad.per_week = ColumnInt(mojom_row, 3);
  creative_ad.per_month = ColumnInt(mojom_row, 4);
  creative_ad.total_max = ColumnInt(mojom_row, 5);
  creative_ad.value = ColumnDouble(mojom_row, 6);
  creative_ad.split_test_group = ColumnString(mojom_row, 13);
  creative_ad.target_url = GURL(ColumnString(mojom_row, 7));

  return creative_ad;
}

CreativeAdList GetCreativeAdsFromResponse(
    mojom::DBStatementResultInfoPtr mojom_statement_result) {
  CHECK(mojom_statement_result);
  CHECK(mojom_statement_result->rows_union);

  CreativeAdMap creative_ads;

  for (const auto& mojom_row : mojom_statement_result->rows_union->get_rows()) {
    const CreativeAdInfo creative_ad = FromMojomRow(&*mojom_row);

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

  CreativeAdList normalized_creative_ads;
  for (const auto& [_, creative_ad] : creative_ads) {
    normalized_creative_ads.push_back(creative_ad);
  }

  return normalized_creative_ads;
}

void GetForCreativeInstanceIdCallback(
    const std::string& creative_instance_id,
    GetCreativeAdCallback callback,
    mojom::DBStatementResultInfoPtr mojom_statement_result) {
  if (!mojom_statement_result ||
      mojom_statement_result->result_code !=
          mojom::DBStatementResultInfo::ResultCode::kSuccess) {
    BLOG(0, "Failed to get creative ad");

    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ad=*/{});
  }

  const CreativeAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(mojom_statement_result));

  if (creative_ads.size() != 1) {
    BLOG(0, "Failed to get creative ad");

    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ad=*/{});
  }

  const CreativeAdInfo& creative_ad = creative_ads.front();

  std::move(callback).Run(/*success=*/true, creative_instance_id, creative_ad);
}

}  // namespace

void CreativeAds::Insert(mojom::DBTransactionInfo* mojom_transaction,
                         const CreativeAdList& creative_ads) {
  CHECK(mojom_transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type = mojom::DBStatementInfo::OperationType::kRun;
  mojom_statement->sql = BuildInsertSql(&*mojom_statement, creative_ads);
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

void CreativeAds::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();

  DeleteTable(&*mojom_transaction, GetTableName());

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void CreativeAds::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativeAdCallback callback) const {
  const CreativeAdInfo creative_ad;

  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   creative_ad);
  }

  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kStep;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
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
            creative_instance_id = '$2';)",
      {GetTableName(), creative_instance_id}, nullptr);
  BindColumnTypes(&*mojom_statement);
  mojom_transaction->statements.push_back(std::move(mojom_statement));

  GetAdsClient()->RunDBTransaction(
      std::move(mojom_transaction),
      base::BindOnce(&GetForCreativeInstanceIdCallback, creative_instance_id,
                     std::move(callback)));
}

std::string CreativeAds::GetTableName() const {
  return kTableName;
}

void CreativeAds::Create(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  Execute(mojom_transaction, R"(
      CREATE TABLE creative_ads (
        creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        creative_set_id TEXT NOT NULL,
        per_day INTEGER NOT NULL DEFAULT 0,
        per_week INTEGER NOT NULL DEFAULT 0,
        per_month INTEGER NOT NULL DEFAULT 0,
        total_max INTEGER NOT NULL DEFAULT 0,
        value DOUBLE NOT NULL DEFAULT 0,
        split_test_group TEXT,
        target_url TEXT NOT NULL
      );)");
}

void CreativeAds::Migrate(mojom::DBTransactionInfo* mojom_transaction,
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

void CreativeAds::MigrateToV43(
    mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // We can safely recreate the table because it will be repopulated after
  // downloading the catalog.
  DropTable(mojom_transaction, GetTableName());
  Create(mojom_transaction);
}

std::string CreativeAds::BuildInsertSql(
    mojom::DBStatementInfo* mojom_statement,
    const CreativeAdList& creative_ads) const {
  CHECK(mojom_statement);
  CHECK(!creative_ads.empty());

  const size_t row_count = BindColumns(mojom_statement, creative_ads);

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
            target_url
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/9, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
