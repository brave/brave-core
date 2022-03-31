/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/creative_new_tab_page_ad_wallpapers_database_table.h"

#include <utility>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"

namespace ads {
namespace database {
namespace table {

namespace {

constexpr char kTableName[] = "creative_new_tab_page_ad_wallpapers";

int BindParameters(mojom::DBCommand* command,
                   const CreativeNewTabPageAdList& creative_ads) {
  DCHECK(command);

  int count = 0;
  int index = 0;

  for (const auto& creative_ad : creative_ads) {
    for (const auto& wallpaper : creative_ad.wallpapers) {
      BindString(command, index++, creative_ad.creative_instance_id);
      BindString(command, index++, wallpaper.image_url);
      BindInt(command, index++, wallpaper.focal_point.x);
      BindInt(command, index++, wallpaper.focal_point.y);
    }

    count += creative_ad.wallpapers.size();
  }

  return count;
}

}  // namespace

CreativeNewTabPageAdWallpapers::CreativeNewTabPageAdWallpapers() = default;

CreativeNewTabPageAdWallpapers::~CreativeNewTabPageAdWallpapers() = default;

void CreativeNewTabPageAdWallpapers::InsertOrUpdate(
    mojom::DBTransaction* transaction,
    const CreativeNewTabPageAdList& creative_ads) {
  DCHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), creative_ads);

  transaction->commands.push_back(std::move(command));
}

void CreativeNewTabPageAdWallpapers::Delete(ResultCallback callback) {
  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  util::Delete(transaction.get(), GetTableName());

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

std::string CreativeNewTabPageAdWallpapers::GetTableName() const {
  return kTableName;
}

void CreativeNewTabPageAdWallpapers::Migrate(mojom::DBTransaction* transaction,
                                             const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 19: {
      MigrateToV19(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

std::string CreativeNewTabPageAdWallpapers::BuildInsertOrUpdateQuery(
    mojom::DBCommand* command,
    const CreativeNewTabPageAdList& creative_ads) {
  DCHECK(command);

  const int count = BindParameters(command, creative_ads);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(creative_instance_id, "
      "image_url, "
      "focal_point_x, "
      "focal_point_y) VALUES %s",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(4, count).c_str());
}

void CreativeNewTabPageAdWallpapers::MigrateToV19(
    mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string& query =
      "CREATE TABLE creative_new_tab_page_ad_wallpapers "
      "(creative_instance_id TEXT NOT NULL, "
      "image_url TEXT NOT NULL, "
      "focal_point_x INT NOT NULL, "
      "focal_point_y INT NOT NULL, "
      "PRIMARY KEY (creative_instance_id, image_url, focal_point_x, "
      "focal_point_y), "
      "UNIQUE(creative_instance_id, image_url, focal_point_x, focal_point_y) "
      "ON CONFLICT REPLACE)";

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace table
}  // namespace database
}  // namespace ads
