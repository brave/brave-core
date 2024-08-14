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
#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"
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

void Create(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  Execute(mojom_transaction, "PRAGMA auto_vacuum = FULL;");

  table::ConfirmationQueue confirmation_queue_database_table;
  confirmation_queue_database_table.Create(mojom_transaction);

  table::AdEvents ad_events_database_table;
  ad_events_database_table.Create(mojom_transaction);

  table::Transactions transactions_database_table;
  transactions_database_table.Create(mojom_transaction);

  table::AdHistory ad_history_database_table;
  ad_history_database_table.Create(mojom_transaction);

  table::Campaigns campaigns_database_table;
  campaigns_database_table.Create(mojom_transaction);

  table::Segments segments_database_table;
  segments_database_table.Create(mojom_transaction);

  table::Deposits deposits_database_table;
  deposits_database_table.Create(mojom_transaction);

  table::CreativeSetConversions creative_set_conversion_database_table;
  creative_set_conversion_database_table.Create(mojom_transaction);

  table::CreativeNotificationAds creative_notification_ads_database_table;
  creative_notification_ads_database_table.Create(mojom_transaction);

  table::CreativeInlineContentAds creative_inline_content_ads_database_table;
  creative_inline_content_ads_database_table.Create(mojom_transaction);

  table::CreativeNewTabPageAds creative_new_tab_page_ads_database_table;
  creative_new_tab_page_ads_database_table.Create(mojom_transaction);

  table::CreativeNewTabPageAdWallpapers
      creative_new_tab_page_ad_wallpapers_database_table;
  creative_new_tab_page_ad_wallpapers_database_table.Create(mojom_transaction);

  table::CreativePromotedContentAds
      creative_promoted_content_ads_database_table;
  creative_promoted_content_ads_database_table.Create(mojom_transaction);

  table::CreativeAds creative_ads_database_table;
  creative_ads_database_table.Create(mojom_transaction);

  table::GeoTargets geo_targets_database_table;
  geo_targets_database_table.Create(mojom_transaction);

  table::Dayparts dayparts_database_table;
  dayparts_database_table.Create(mojom_transaction);
}

}  // namespace

void Create(ResultCallback callback) {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom_transaction->version = kVersion;
  mojom_transaction->compatible_version = kCompatibleVersion;

  Create(&*mojom_transaction);

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

}  // namespace brave_ads::database
