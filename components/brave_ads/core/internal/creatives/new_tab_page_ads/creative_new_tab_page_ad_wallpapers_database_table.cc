/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpapers_database_table.h"

#include <iterator>
#include <utility>

#include "base/check.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "creative_new_tab_page_ad_wallpapers";

size_t BindParameters(mojom::DBCommandInfo* command,
                      const CreativeNewTabPageAdList& creative_ads) {
  CHECK(command);

  size_t count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    for (const auto& wallpaper : creative_ad.wallpapers) {
      BindString(command, index++, creative_ad.creative_instance_id);
      BindString(command, index++, wallpaper.image_url.spec());
      BindInt(command, index++, wallpaper.focal_point.x);
      BindInt(command, index++, wallpaper.focal_point.y);
    }

    count += creative_ad.wallpapers.size();
  }

  return count;
}

void MigrateToV24(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  DropTable(transaction, "creative_new_tab_page_ad_wallpapers");

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE creative_new_tab_page_ad_wallpapers (creative_instance_id "
      "TEXT NOT NULL, image_url TEXT NOT NULL, focal_point_x INT NOT NULL, "
      "focal_point_y INT NOT NULL, PRIMARY KEY (creative_instance_id, "
      "image_url, focal_point_x, focal_point_y), UNIQUE(creative_instance_id, "
      "image_url, focal_point_x, focal_point_y) ON CONFLICT REPLACE);";
  transaction->commands.push_back(std::move(command));
}

}  // namespace

void CreativeNewTabPageAdWallpapers::InsertOrUpdate(
    mojom::DBTransactionInfo* transaction,
    const CreativeNewTabPageAdList& creative_ads) {
  CHECK(transaction);

  CreativeNewTabPageAdList filtered_creative_ads;
  base::ranges::copy_if(creative_ads, std::back_inserter(filtered_creative_ads),
                        [](const CreativeNewTabPageAdInfo& creative_ad) {
                          return !creative_ad.wallpapers.empty();
                        });

  if (filtered_creative_ads.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, filtered_creative_ads);
  transaction->commands.push_back(std::move(command));
}

void CreativeNewTabPageAdWallpapers::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(&*transaction, GetTableName());

  RunTransaction(std::move(transaction), std::move(callback));
}

std::string CreativeNewTabPageAdWallpapers::GetTableName() const {
  return kTableName;
}

void CreativeNewTabPageAdWallpapers::Create(
    mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE creative_new_tab_page_ad_wallpapers "
      "(creative_instance_id TEXT NOT NULL, image_url TEXT NOT NULL, "
      "focal_point_x INT NOT NULL, focal_point_y INT NOT NULL, PRIMARY KEY "
      "(creative_instance_id, image_url, focal_point_x, focal_point_y), "
      "UNIQUE(creative_instance_id, image_url, focal_point_x, focal_point_y) "
      "ON CONFLICT REPLACE);";
  transaction->commands.push_back(std::move(command));
}

void CreativeNewTabPageAdWallpapers::Migrate(
    mojom::DBTransactionInfo* transaction,
    const int to_version) {
  CHECK(transaction);

  switch (to_version) {
    case 24: {
      MigrateToV24(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

std::string CreativeNewTabPageAdWallpapers::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const CreativeNewTabPageAdList& creative_ads) const {
  CHECK(command);

  const size_t binded_parameters_count = BindParameters(command, creative_ads);

  return base::ReplaceStringPlaceholders(
      "INSERT OR REPLACE INTO $1 (creative_instance_id, image_url, "
      "focal_point_x, focal_point_y) VALUES $2;",
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count*/ 4, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
