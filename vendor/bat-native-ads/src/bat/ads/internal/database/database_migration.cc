/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/database_migration.h"

#include <functional>
#include <utility>

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/database/database_version.h"
#include "bat/ads/internal/database/tables/ad_conversions_database_table.h"
#include "bat/ads/internal/database/tables/categories_database_table.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/database/tables/geo_targets_database_table.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace database {

using std::placeholders::_1;

Migration::Migration(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

Migration::~Migration() = default;

void Migration::FromVersion(
    const int from_version,
    ResultCallback callback) {
  const int to_version = version();
  if (to_version == from_version) {
    callback(Result::SUCCESS);
    return;
  }

  DBTransactionPtr transaction = DBTransaction::New();
  for (int i = from_version + 1; i <= to_version; i++) {
    ToVersion(transaction.get(), i);
  }

  BLOG(1, "Migrated database from version " << from_version
      << " to version " << to_version);

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::MIGRATE;

  transaction->version = to_version;
  transaction->compatible_version = compatible_version();
  transaction->commands.push_back(std::move(command));

  ads_->get_ads_client()->RunDBTransaction(std::move(transaction),
      std::bind(&OnResultCallback, _1, callback));
}

void Migration::ToVersion(
    DBTransaction* transaction,
    const int to_version) {
  DCHECK(transaction);

  table::AdConversions ad_conversions_database_table(ads_);
  ad_conversions_database_table.Migrate(transaction, to_version);

  table::Categories categories_database_table(ads_);
  categories_database_table.Migrate(transaction, to_version);

  table::CreativeAdNotifications creative_ad_notifications_database_table(ads_);
  creative_ad_notifications_database_table.Migrate(transaction, to_version);

  table::GeoTargets geo_targets_database_table(ads_);
  geo_targets_database_table.Migrate(transaction, to_version);
}

}  // namespace database
}  // namespace ads
