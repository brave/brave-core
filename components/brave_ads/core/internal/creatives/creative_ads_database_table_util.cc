/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table_util.h"

#include <cstddef>
#include <cstdint>
#include <utility>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/creatives/condition_matchers_database_table_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "creative_ads";

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
    BindColumnString(mojom_db_action, index++,
                     ConditionMatchersToString(creative_ad.condition_matchers));
    BindColumnString(mojom_db_action, index++, creative_ad.target_url.spec());

    ++row_count;
  }

  return row_count;
}

std::string BuildInsertSql(const mojom::DBActionInfoPtr& mojom_db_action,
                           const CreativeAdList& creative_ads) {
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
            condition_matchers,
            target_url
          ) VALUES $2)",
      {kTableName, BuildBindColumnPlaceholders(/*column_count=*/9, row_count)},
      nullptr);
}

}  // namespace

CreativeAdInfo CreativeAdFromMojomRow(const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);
  CHECK_EQ(9U, mojom_db_row->column_values_union.size());

  CreativeAdInfo creative_ad;

  creative_ad.creative_instance_id = ColumnString(mojom_db_row, 0);
  creative_ad.creative_set_id = ColumnString(mojom_db_row, 1);
  creative_ad.per_day = ColumnInt(mojom_db_row, 2);
  creative_ad.per_week = ColumnInt(mojom_db_row, 3);
  creative_ad.per_month = ColumnInt(mojom_db_row, 4);
  creative_ad.total_max = ColumnInt(mojom_db_row, 5);
  creative_ad.value = ColumnDouble(mojom_db_row, 6);
  creative_ad.condition_matchers =
      StringToConditionMatchers(ColumnString(mojom_db_row, 7));
  creative_ad.target_url = GURL(ColumnString(mojom_db_row, 8));

  return creative_ad;
}

void InsertCreativeAds(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
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

}  // namespace brave_ads::database::table
