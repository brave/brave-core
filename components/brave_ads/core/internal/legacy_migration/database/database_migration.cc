/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_migration.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/creatives/campaigns_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/dayparts_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/geo_targets_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpapers_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/segments_database_table.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_database_table.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_constants.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database {

namespace {

void MigrateToV44(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Normally, whether or not the database supports `auto_vacuum` must be
  // configured before the database file is actually created. However, when not
  // in write-ahead log mode, the `auto_vacuum` properties of an existing
  // database may be changed by using the `auto_vacuum` pragmas and then
  // immediately VACUUMing the database.

  Execute(mojom_db_transaction, "PRAGMA auto_vacuum = FULL;");
  Vacuum(mojom_db_transaction);
}

void Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
             const int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 44: {
      MigrateToV44(mojom_db_transaction);
      break;
    }
  }
}

void MigrateToVersion(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                      const int to_version) {
  CHECK(mojom_db_transaction);

  table::CreativeSetConversions creative_set_conversion_database_table;
  creative_set_conversion_database_table.Migrate(mojom_db_transaction,
                                                 to_version);

  table::ConfirmationQueue confirmation_queue_database_table;
  confirmation_queue_database_table.Migrate(mojom_db_transaction, to_version);

  table::AdEvents ad_events_database_table;
  ad_events_database_table.Migrate(mojom_db_transaction, to_version);

  table::Transactions transactions_database_table;
  transactions_database_table.Migrate(mojom_db_transaction, to_version);

  table::AdHistory ad_history_database_table;
  ad_history_database_table.Migrate(mojom_db_transaction, to_version);

  table::Campaigns campaigns_database_table;
  campaigns_database_table.Migrate(mojom_db_transaction, to_version);

  table::Segments segments_database_table;
  segments_database_table.Migrate(mojom_db_transaction, to_version);

  table::Deposits deposits_database_table;
  deposits_database_table.Migrate(mojom_db_transaction, to_version);

  table::CreativeNotificationAds creative_notification_ads_database_table;
  creative_notification_ads_database_table.Migrate(mojom_db_transaction,
                                                   to_version);

  table::CreativeInlineContentAds creative_inline_content_ads_database_table;
  creative_inline_content_ads_database_table.Migrate(mojom_db_transaction,
                                                     to_version);

  table::CreativeNewTabPageAds creative_new_tab_page_ads_database_table;
  creative_new_tab_page_ads_database_table.Migrate(mojom_db_transaction,
                                                   to_version);

  table::CreativeNewTabPageAdWallpapers
      creative_new_tab_page_ad_wallpapers_database_table;
  creative_new_tab_page_ad_wallpapers_database_table.Migrate(
      mojom_db_transaction, to_version);

  table::CreativePromotedContentAds
      creative_promoted_content_ads_database_table;
  creative_promoted_content_ads_database_table.Migrate(mojom_db_transaction,
                                                       to_version);

  table::CreativeAds creative_ads_database_table;
  creative_ads_database_table.Migrate(mojom_db_transaction, to_version);

  table::GeoTargets geo_targets_database_table;
  geo_targets_database_table.Migrate(mojom_db_transaction, to_version);

  table::Dayparts dayparts_database_table;
  dayparts_database_table.Migrate(mojom_db_transaction, to_version);

  Migrate(mojom_db_transaction, to_version);
}

}  // namespace

void MigrateFromVersion(const int from_version, ResultCallback callback) {
  CHECK_LT(from_version, kVersionNumber);

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  for (int i = from_version + 1; i <= kVersionNumber; ++i) {
    MigrateToVersion(mojom_db_transaction, i);
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kMigrate;
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunDBTransaction(std::move(mojom_db_transaction), std::move(callback));
}

}  // namespace brave_ads::database
