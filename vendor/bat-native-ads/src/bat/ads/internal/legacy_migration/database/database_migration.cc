/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/database/database_migration.h"

#include <functional>
#include <utility>

#include "base/check.h"
#include "bat/ads/internal/account/deposits/deposits_database_table.h"
#include "bat/ads/internal/account/transactions/transactions_database_table.h"
#include "bat/ads/internal/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ad_server/catalog/bundle/campaigns_database_table.h"
#include "bat/ads/internal/ad_server/catalog/bundle/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/ad_server/catalog/bundle/creative_ads_database_table.h"
#include "bat/ads/internal/ad_server/catalog/bundle/creative_inline_content_ads_database_table.h"
#include "bat/ads/internal/ad_server/catalog/bundle/creative_new_tab_page_ad_wallpapers_database_table.h"
#include "bat/ads/internal/ad_server/catalog/bundle/creative_new_tab_page_ads_database_table.h"
#include "bat/ads/internal/ad_server/catalog/bundle/creative_promoted_content_ads_database_table.h"
#include "bat/ads/internal/ad_server/catalog/bundle/dayparts_database_table.h"
#include "bat/ads/internal/ad_server/catalog/bundle/geo_targets_database_table.h"
#include "bat/ads/internal/ad_server/catalog/bundle/segments_database_table.h"
#include "bat/ads/internal/ad_server/catalog/catalog_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/database_util.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/conversions/conversion_queue_database_table.h"
#include "bat/ads/internal/conversions/conversions_database_table.h"
#include "bat/ads/internal/legacy_migration/database/database_version.h"

namespace ads {
namespace database {

Migration::Migration() = default;

Migration::~Migration() = default;

void Migration::FromVersion(const int from_version, ResultCallback callback) {
  const int to_version = version();
  if (to_version == from_version) {
    callback(/* success */ true);
    return;
  }

  // TODO(https://github.com/brave/brave-browser/issues/14728): Decouple catalog
  // business logic once we have implemented database observers
  ResetCatalog();

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  for (int i = from_version + 1; i <= to_version; i++) {
    ToVersion(transaction.get(), i);
  }

  BLOG(1, "Migrated database from version " << from_version << " to version "
                                            << to_version);

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::MIGRATE;

  transaction->version = to_version;
  transaction->compatible_version = compatible_version();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void Migration::ToVersion(mojom::DBTransaction* transaction,
                          const int to_version) {
  DCHECK(transaction);

  table::Conversions conversions_database_table;
  conversions_database_table.Migrate(transaction, to_version);

  table::ConversionQueue conversion_queue_database_table;
  conversion_queue_database_table.Migrate(transaction, to_version);

  table::AdEvents ad_events_database_table;
  ad_events_database_table.Migrate(transaction, to_version);

  table::Transactions transactions_database_table;
  transactions_database_table.Migrate(transaction, to_version);

  table::Campaigns campaigns_database_table;
  campaigns_database_table.Migrate(transaction, to_version);

  table::Segments segments_database_table;
  segments_database_table.Migrate(transaction, to_version);

  table::Deposits deposits_database_table;
  deposits_database_table.Migrate(transaction, to_version);

  table::CreativeAdNotifications creative_ad_notifications_database_table;
  creative_ad_notifications_database_table.Migrate(transaction, to_version);

  table::CreativeInlineContentAds creative_inline_content_ads_database_table;
  creative_inline_content_ads_database_table.Migrate(transaction, to_version);

  table::CreativeNewTabPageAds creative_new_tab_page_ads_database_table;
  creative_new_tab_page_ads_database_table.Migrate(transaction, to_version);

  table::CreativeNewTabPageAdWallpapers
      creative_new_tab_page_ad_wallpapers_database_table;
  creative_new_tab_page_ad_wallpapers_database_table.Migrate(transaction,
                                                             to_version);

  table::CreativePromotedContentAds
      creative_promoted_content_ads_database_table;
  creative_promoted_content_ads_database_table.Migrate(transaction, to_version);

  table::CreativeAds creative_ads_database_table;
  creative_ads_database_table.Migrate(transaction, to_version);

  table::GeoTargets geo_targets_database_table;
  geo_targets_database_table.Migrate(transaction, to_version);

  table::Dayparts dayparts_database_table;
  dayparts_database_table.Migrate(transaction, to_version);
}

}  // namespace database
}  // namespace ads
