/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpapers_database_table.h"

#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/location.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "creative_new_tab_page_ad_wallpapers";

std::string ConditionMatchersToString(
    const ConditionMatcherMap& condition_matchers) {
  std::vector<std::string> condition_matchers_as_string;
  condition_matchers_as_string.reserve(condition_matchers.size());

  for (const auto& [pref_name, condition] : condition_matchers) {
    // We base64 encode the `pref_name` and `condition` to avoid any issues with
    // pref paths and conditions that contain either `|` or `;`.

    const std::string condition_matcher = base::StrCat(
        {base::Base64Encode(pref_name), "|", base::Base64Encode(condition)});
    condition_matchers_as_string.push_back(condition_matcher);
  }

  return base::JoinString(condition_matchers_as_string, ";");
}

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const CreativeNewTabPageAdList& creative_ads) {
  CHECK(mojom_db_action);
  CHECK(!creative_ads.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    for (const auto& wallpaper : creative_ad.wallpapers) {
      BindColumnString(mojom_db_action, index++,
                       creative_ad.creative_instance_id);
      BindColumnString(mojom_db_action, index++, wallpaper.image_url.spec());
      BindColumnInt(mojom_db_action, index++, wallpaper.focal_point.x);
      BindColumnInt(mojom_db_action, index++, wallpaper.focal_point.y);
      BindColumnString(mojom_db_action, index++,
                       ConditionMatchersToString(wallpaper.condition_matchers));
    }

    row_count += creative_ad.wallpapers.size();
  }

  return row_count;
}

}  // namespace

void CreativeNewTabPageAdWallpapers::Insert(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    const CreativeNewTabPageAdList& creative_ads) {
  CHECK(mojom_db_transaction);

  CreativeNewTabPageAdList filtered_creative_ads;
  base::ranges::copy_if(creative_ads, std::back_inserter(filtered_creative_ads),
                        [](const CreativeNewTabPageAdInfo& creative_ad) {
                          return !creative_ad.wallpapers.empty();
                        });

  if (filtered_creative_ads.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, filtered_creative_ads);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

void CreativeNewTabPageAdWallpapers::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  DeleteTable(mojom_db_transaction, GetTableName());

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   std::move(callback));
}

std::string CreativeNewTabPageAdWallpapers::GetTableName() const {
  return kTableName;
}

void CreativeNewTabPageAdWallpapers::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE creative_new_tab_page_ad_wallpapers (
        creative_instance_id TEXT NOT NULL,
        image_url TEXT NOT NULL,
        focal_point_x INT NOT NULL,
        focal_point_y INT NOT NULL,
        condition_matchers TEXT NOT NULL,
        PRIMARY KEY (
          creative_instance_id,
          image_url,
          focal_point_x,
          focal_point_y,
          condition_matchers
        ) ON CONFLICT REPLACE
      );)");
}

void CreativeNewTabPageAdWallpapers::Migrate(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction,
    int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 45: {
      MigrateToV45(mojom_db_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeNewTabPageAdWallpapers::MigrateToV45(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // We can safely recreate the table because it will be repopulated after
  // downloading the catalog.
  DropTable(mojom_db_transaction, GetTableName());
  Create(mojom_db_transaction);
}

std::string CreativeNewTabPageAdWallpapers::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const CreativeNewTabPageAdList& creative_ads) const {
  CHECK(mojom_db_action);
  CHECK(!creative_ads.empty());

  const size_t row_count = BindColumns(mojom_db_action, creative_ads);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_instance_id,
            image_url,
            focal_point_x,
            focal_point_y,
            condition_matchers
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/5, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
