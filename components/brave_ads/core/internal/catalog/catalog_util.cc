/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_feature.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creatives_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/creatives_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

namespace {

void Delete(ResultCallback callback) {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  // Remove catalog data not linked to new tab page ads. New tab page ads are
  // provided by component resources and will be purged after the campaign ends.
  database::Execute(mojom_db_transaction, R"(
      DELETE FROM
        geo_targets
      WHERE
        campaign_id NOT IN (
          SELECT
            DISTINCT campaign_id
          FROM
            creative_new_tab_page_ads
        );

      DELETE FROM
        segments
      WHERE
        creative_set_id NOT IN (
          SELECT
            DISTINCT creative_set_id
          FROM
            creative_new_tab_page_ads
        );

      DELETE FROM
        dayparts
      WHERE
        campaign_id NOT IN (
          SELECT
            DISTINCT campaign_id
          FROM
            creative_new_tab_page_ads
        );

      DELETE FROM
        creative_promoted_content_ads
      WHERE
        creative_instance_id NOT IN (
          SELECT
            DISTINCT creative_instance_id
          FROM
            creative_new_tab_page_ads
        );

      DELETE FROM
        creative_inline_content_ads
      WHERE
        creative_instance_id NOT IN (
          SELECT
            DISTINCT creative_instance_id
          FROM
            creative_new_tab_page_ads
        );

      DELETE FROM
        creative_ads
      WHERE
        creative_instance_id NOT IN (
          SELECT
            DISTINCT creative_instance_id
          FROM
            creative_new_tab_page_ads
        );

      DELETE FROM
        creative_ad_notifications
      WHERE
        creative_instance_id NOT IN (
          SELECT
            DISTINCT creative_instance_id
          FROM
            creative_new_tab_page_ads
        );

      DELETE FROM
        campaigns
      WHERE
        id NOT IN (
          SELECT
            DISTINCT campaign_id
          FROM
            creative_new_tab_page_ads
        );)");

  database::RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                             std::move(callback));
}

void SaveCatalogCallback(const CatalogInfo& catalog, bool success) {
  if (!success) {
    return BLOG(0, "Failed to save catalog");
  }

  SetCatalogId(catalog.id);
  SetCatalogVersion(catalog.version);
  SetCatalogPing(catalog.ping);

  const CreativesInfo creatives = BuildCreatives(catalog);
  database::SaveCreativeNotificationAds(creatives.notification_ads);
  database::SaveCreativeInlineContentAds(creatives.inline_content_ads);
  database::SaveCreativePromotedContentAds(creatives.promoted_content_ads);
  database::SaveCreativeSetConversions(creatives.conversions);
}

void ResetCatalogCallback(bool success) {
  if (!success) {
    return BLOG(0, "Failed to reset catalog");
  }

  ClearProfilePref(prefs::kCatalogId);
  ClearProfilePref(prefs::kCatalogVersion);
  ClearProfilePref(prefs::kCatalogPing);
  ClearProfilePref(prefs::kCatalogLastUpdated);
}

}  // namespace

void SaveCatalog(const CatalogInfo& catalog) {
  Delete(base::BindOnce(&SaveCatalogCallback, catalog));
}

void ResetCatalog() {
  Delete(base::BindOnce(&ResetCatalogCallback));
}

std::string GetCatalogId() {
  return GetProfileStringPref(prefs::kCatalogId);
}

void SetCatalogId(const std::string& id) {
  SetProfileStringPref(prefs::kCatalogId, id);
}

int GetCatalogVersion() {
  return GetProfileIntegerPref(prefs::kCatalogVersion);
}

void SetCatalogVersion(int version) {
  SetProfileIntegerPref(prefs::kCatalogVersion, version);
}

base::TimeDelta GetCatalogPing() {
  return base::Milliseconds(GetProfileInt64Pref(prefs::kCatalogPing));
}

void SetCatalogPing(base::TimeDelta ping) {
  SetProfileInt64Pref(prefs::kCatalogPing, ping.InMilliseconds());
}

base::Time GetCatalogLastUpdated() {
  return GetProfileTimePref(prefs::kCatalogLastUpdated);
}

void SetCatalogLastUpdated(base::Time last_updated_at) {
  SetProfileTimePref(prefs::kCatalogLastUpdated, last_updated_at);
}

bool DoesCatalogExist() {
  return GetCatalogVersion() > 0;
}

bool HasCatalogChanged(const std::string& catalog_id) {
  return catalog_id != GetCatalogId();
}

bool HasCatalogExpired() {
  return base::Time::Now() >= GetCatalogLastUpdated() + kCatalogLifespan.Get();
}

}  // namespace brave_ads
