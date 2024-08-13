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

size_t BindColumns(mojom::DBStatementInfo* mojom_statement,
                   const CreativeNewTabPageAdList& creative_ads) {
  CHECK(mojom_statement);
  CHECK(!creative_ads.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    for (const auto& wallpaper : creative_ad.wallpapers) {
      BindColumnString(mojom_statement, index++,
                       creative_ad.creative_instance_id);
      BindColumnString(mojom_statement, index++, wallpaper.image_url.spec());
      BindColumnInt(mojom_statement, index++, wallpaper.focal_point.x);
      BindColumnInt(mojom_statement, index++, wallpaper.focal_point.y);
    }

    row_count += creative_ad.wallpapers.size();
  }

  return row_count;
}

}  // namespace

void CreativeNewTabPageAdWallpapers::Insert(
    mojom::DBTransactionInfo* mojom_transaction,
    const CreativeNewTabPageAdList& creative_ads) {
  CHECK(mojom_transaction);

  CreativeNewTabPageAdList filtered_creative_ads;
  base::ranges::copy_if(creative_ads, std::back_inserter(filtered_creative_ads),
                        [](const CreativeNewTabPageAdInfo& creative_ad) {
                          return !creative_ad.wallpapers.empty();
                        });

  if (filtered_creative_ads.empty()) {
    return;
  }

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type = mojom::DBStatementInfo::OperationType::kRun;
  mojom_statement->sql =
      BuildInsertSql(&*mojom_statement, filtered_creative_ads);
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

void CreativeNewTabPageAdWallpapers::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();

  DeleteTable(&*mojom_transaction, GetTableName());

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

std::string CreativeNewTabPageAdWallpapers::GetTableName() const {
  return kTableName;
}

void CreativeNewTabPageAdWallpapers::Create(
    mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kExecute;
  mojom_statement->sql =
      R"(
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
          );)";
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

void CreativeNewTabPageAdWallpapers::Migrate(
    mojom::DBTransactionInfo* mojom_transaction,
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

void CreativeNewTabPageAdWallpapers::MigrateToV43(
    mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // We can safely recreate the table because it will be repopulated after
  // downloading the catalog.
  DropTable(mojom_transaction, GetTableName());
  Create(mojom_transaction);
}

std::string CreativeNewTabPageAdWallpapers::BuildInsertSql(
    mojom::DBStatementInfo* mojom_statement,
    const CreativeNewTabPageAdList& creative_ads) const {
  CHECK(mojom_statement);
  CHECK(!creative_ads.empty());

  const size_t row_count = BindColumns(mojom_statement, creative_ads);

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
