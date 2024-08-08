/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_creation.h"

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

void Create(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  table::ConfirmationQueue confirmation_queue_database_table;
  confirmation_queue_database_table.Create(transaction);

  table::AdEvents ad_events_database_table;
  ad_events_database_table.Create(transaction);

  table::Transactions transactions_database_table;
  transactions_database_table.Create(transaction);

  table::AdHistory ad_history_database_table;
  ad_history_database_table.Create(transaction);

  table::Campaigns campaigns_database_table;
  campaigns_database_table.Create(transaction);

  table::Segments segments_database_table;
  segments_database_table.Create(transaction);

  table::Deposits deposits_database_table;
  deposits_database_table.Create(transaction);

  table::CreativeSetConversions creative_set_conversion_database_table;
  creative_set_conversion_database_table.Create(transaction);

  table::CreativeNotificationAds creative_notification_ads_database_table;
  creative_notification_ads_database_table.Create(transaction);

  table::CreativeInlineContentAds creative_inline_content_ads_database_table;
  creative_inline_content_ads_database_table.Create(transaction);

  table::CreativeNewTabPageAds creative_new_tab_page_ads_database_table;
  creative_new_tab_page_ads_database_table.Create(transaction);

  table::CreativeNewTabPageAdWallpapers
      creative_new_tab_page_ad_wallpapers_database_table;
  creative_new_tab_page_ad_wallpapers_database_table.Create(transaction);

  table::CreativePromotedContentAds
      creative_promoted_content_ads_database_table;
  creative_promoted_content_ads_database_table.Create(transaction);

  table::CreativeAds creative_ads_database_table;
  creative_ads_database_table.Create(transaction);

  table::GeoTargets geo_targets_database_table;
  geo_targets_database_table.Create(transaction);

  table::Dayparts dayparts_database_table;
  dayparts_database_table.Create(transaction);
}

}  // namespace

void Create(ResultCallback callback) {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  Create(&*transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::MIGRATE;

  transaction->version = kVersion;
  transaction->compatible_version = kCompatibleVersion;
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

}  // namespace brave_ads::database
