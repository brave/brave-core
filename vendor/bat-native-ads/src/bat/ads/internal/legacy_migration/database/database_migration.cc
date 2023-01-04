/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/database/database_migration.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "bat/ads/internal/account/deposits/deposits_database_table.h"
#include "bat/ads/internal/account/transactions/transactions_database_table.h"
#include "bat/ads/internal/ads/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/database/database_transaction_util.h"
#include "bat/ads/internal/conversions/conversion_queue_database_table.h"
#include "bat/ads/internal/conversions/conversions_database_table.h"
#include "bat/ads/internal/creatives/campaigns_database_table.h"
#include "bat/ads/internal/creatives/creative_ads_database_table.h"
#include "bat/ads/internal/creatives/dayparts_database_table.h"
#include "bat/ads/internal/creatives/geo_targets_database_table.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpapers_database_table.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ads_database_table.h"
#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_table.h"
#include "bat/ads/internal/creatives/segments_database_table.h"
#include "bat/ads/internal/legacy_migration/database/database_constants.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events_database_table.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads::database {

namespace {

void MigrateToVersion(mojom::DBTransactionInfo* transaction,
                      const int to_version) {
  DCHECK(transaction);

  table::Conversions conversions_database_table;
  conversions_database_table.Migrate(transaction, to_version);

  table::ConversionQueue conversion_queue_database_table;
  conversion_queue_database_table.Migrate(transaction, to_version);

  table::AdEvents ad_events_database_table;
  ad_events_database_table.Migrate(transaction, to_version);

  table::TextEmbeddingHtmlEvents text_embedding_html_events_database_table;
  text_embedding_html_events_database_table.Migrate(transaction, to_version);

  table::Transactions transactions_database_table;
  transactions_database_table.Migrate(transaction, to_version);

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
  const int to_version = database::kVersion;
  DCHECK(from_version < to_version);

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  for (int i = from_version + 1; i <= to_version; i++) {
    MigrateToVersion(transaction.get(), i);
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::MIGRATE;

  transaction->version = to_version;
  transaction->compatible_version = database::kCompatibleVersion;
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

}  // namespace ads::database
