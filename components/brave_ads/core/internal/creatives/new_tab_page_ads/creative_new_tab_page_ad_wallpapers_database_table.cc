/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpapers_database_table.h"

#include <cstddef>
#include <iterator>
#include <utility>

#include "base/check.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "creative_new_tab_page_ad_wallpapers";

size_t BindColumns(mojom::DBActionInfo* mojom_db_action,
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
    }

    row_count += creative_ad.wallpapers.size();
  }

  return row_count;
}

}  // namespace

void CreativeNewTabPageAdWallpapers::Insert(
    mojom::DBTransactionInfo* mojom_db_transaction,
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
  mojom_db_action->type = mojom::DBActionInfo::Type::kRunStatement;
  mojom_db_action->sql =
      BuildInsertSql(&*mojom_db_action, filtered_creative_ads);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

void CreativeNewTabPageAdWallpapers::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  DeleteTable(&*mojom_db_transaction, GetTableName());

  RunDBTransaction(std::move(mojom_db_transaction), std::move(callback));
}

std::string CreativeNewTabPageAdWallpapers::GetTableName() const {
  return kTableName;
}

void CreativeNewTabPageAdWallpapers::Create(
    mojom::DBTransactionInfo* const mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE creative_new_tab_page_ad_wallpapers (
        creative_instance_id TEXT NOT NULL,
        image_url TEXT NOT NULL,
        focal_point_x INT NOT NULL,
        focal_point_y INT NOT NULL,
        PRIMARY KEY (
          creative_instance_id,
          image_url,
          focal_point_x,
          focal_point_y
        ) ON CONFLICT REPLACE
      );)");
}

void CreativeNewTabPageAdWallpapers::Migrate(
    mojom::DBTransactionInfo* mojom_db_transaction,
    const int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 43: {
      MigrateToV43(mojom_db_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeNewTabPageAdWallpapers::MigrateToV43(
    mojom::DBTransactionInfo* const mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // We can safely recreate the table because it will be repopulated after
  // downloading the catalog.
  DropTable(mojom_db_transaction, GetTableName());
  Create(mojom_db_transaction);
}

std::string CreativeNewTabPageAdWallpapers::BuildInsertSql(
    mojom::DBActionInfo* mojom_db_action,
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
            focal_point_y
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/4, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
