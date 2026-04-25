/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_feature.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creatives_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/creatives_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

namespace {

void Delete(ResultCallback callback) {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  // Remove campaigns, geo targets, dayparts, segments, and creative ads
  // not associated with new tab page ads. New tab page ads are provided by
  // component resources.

  database::Execute(mojom_db_transaction, R"(
      DELETE FROM
        campaigns
      WHERE
        id NOT IN (
          SELECT
            DISTINCT campaign_id
          FROM
            creative_new_tab_page_ads
        ))");

  database::Execute(mojom_db_transaction, R"(
      DELETE FROM
        geo_targets
      WHERE
        campaign_id NOT IN (
          SELECT
            DISTINCT campaign_id
          FROM
            creative_new_tab_page_ads
        ))");

  database::Execute(mojom_db_transaction, R"(
      DELETE FROM
        dayparts
      WHERE
        campaign_id NOT IN (
          SELECT
            DISTINCT campaign_id
          FROM
            creative_new_tab_page_ads
        ))");

  database::Execute(mojom_db_transaction, R"(
      DELETE FROM
        segments
      WHERE
        creative_set_id NOT IN (
          SELECT
            DISTINCT creative_set_id
          FROM
            creative_new_tab_page_ads
        ))");

  database::Execute(mojom_db_transaction, R"(
      DELETE FROM
        creative_ads
      WHERE
        creative_instance_id NOT IN (
          SELECT
            DISTINCT creative_instance_id
          FROM
            creative_new_tab_page_ads
        ))");

  database::Execute(mojom_db_transaction, R"(
      DELETE FROM
        creative_ad_notifications
      WHERE
        creative_instance_id NOT IN (
          SELECT
            DISTINCT creative_instance_id
          FROM
            creative_new_tab_page_ads
        ))");

  database::RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                           std::move(callback));
}

void SaveCatalogCallback(const CatalogInfo& catalog,
                         ResultCallback callback,
                         bool success) {
  if (success) {
    SetCatalogId(catalog.id);
    SetCatalogVersion(catalog.version);

    const CreativesInfo creatives = BuildCreatives(catalog);
    database::SaveCreativeNotificationAds(creatives.notification_ads);
    database::SaveCreativeSetConversions(creatives.conversions);
  } else {
    BLOG(0, "Failed to save catalog");
  }

  std::move(callback).Run(success);
}

void ResetCatalogCallback(ResultCallback callback, bool success) {
  if (success) {
    ClearProfilePref(prefs::kCatalogId);
    ClearProfilePref(prefs::kCatalogVersion);
    ClearProfilePref(prefs::kCatalogPing);
    ClearProfilePref(prefs::kCatalogLastUpdated);
  } else {
    BLOG(0, "Failed to reset catalog");
  }

  std::move(callback).Run(success);
}

}  // namespace

void SaveCatalog(const CatalogInfo& catalog, ResultCallback callback) {
  Delete(base::BindOnce(&SaveCatalogCallback, catalog, std::move(callback)));
}

void ResetCatalog(ResultCallback callback) {
  Delete(base::BindOnce(&ResetCatalogCallback, std::move(callback)));
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
