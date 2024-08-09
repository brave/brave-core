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

void MigrateToVersion(mojom::DBTransactionInfo* transaction,
                      const int to_version) {
  CHECK(transaction);

  table::CreativeSetConversions creative_set_conversion_database_table;
  creative_set_conversion_database_table.Migrate(transaction, to_version);

  table::ConfirmationQueue confirmation_queue_database_table;
  confirmation_queue_database_table.Migrate(transaction, to_version);

  table::AdEvents ad_events_database_table;
  ad_events_database_table.Migrate(transaction, to_version);

  table::Transactions transactions_database_table;
  transactions_database_table.Migrate(transaction, to_version);

  table::AdHistory ad_history_database_table;
  ad_history_database_table.Migrate(transaction, to_version);

  table::Campaigns campaigns_database_table;
  campaigns_database_table.Migrate(transaction, to_version);

  table::Segments segments_database_table;
  segments_database_table.Migrate(transaction, to_version);

  table::Deposits deposits_database_table;
  deposits_database_table.Migrate(transaction, to_version);

  table::CreativeNotificationAds creative_notification_ads_database_table;
  creative_notification_ads_database_table.Migrate(transaction, to_version);

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

}  // namespace

void MigrateFromVersion(const int from_version, ResultCallback callback) {
  CHECK_LT(from_version, kVersion);

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  for (int i = from_version + 1; i <= kVersion; ++i) {
    MigrateToVersion(&*transaction, i);
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::MIGRATE;

  transaction->version = kVersion;
  transaction->compatible_version = kCompatibleVersion;
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

}  // namespace brave_ads::database
