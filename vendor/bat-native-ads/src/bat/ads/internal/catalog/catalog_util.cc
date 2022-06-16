/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_util.h"

#include <cstdint>

#include "base/time/time.h"
#include "bat/ads/internal/account/deposits/deposits_database_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/catalog/catalog_info.h"
#include "bat/ads/internal/conversions/conversions_database_util.h"
#include "bat/ads/internal/creatives/campaigns_database_util.h"
#include "bat/ads/internal/creatives/creative_ads_database_util.h"
#include "bat/ads/internal/creatives/creatives_builder.h"
#include "bat/ads/internal/creatives/creatives_info.h"
#include "bat/ads/internal/creatives/dayparts_database_util.h"
#include "bat/ads/internal/creatives/geo_targets_database_util.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_util.h"
#include "bat/ads/internal/creatives/segments_database_util.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {

constexpr base::TimeDelta kCatalogLifespan = base::Days(1);

void Delete() {
  database::DeleteCampaigns();
  database::DeleteCreativeNotificationAds();
  database::DeleteCreativeInlineContentAds();
  database::DeleteCreativeNewTabPageAds();
  database::DeleteCreativeNewTabPageAdWallpapers();
  database::DeleteCreativePromotedContentAds();
  database::DeleteCreativeAds();
  database::DeleteSegments();
  database::DeleteGeoTargets();
  database::DeleteDayparts();
}

void PurgeExpired() {
  database::PurgeExpiredConversions();
  database::PurgeExpiredDeposits();
}

}  // namespace

void SaveCatalog(const CatalogInfo& catalog) {
  Delete();

  PurgeExpired();

  SetCatalogId(catalog.id);
  SetCatalogVersion(catalog.version);
  SetCatalogPing(catalog.ping);

  const CreativesInfo creatives = BuildCreatives(catalog);
  database::SaveCreativeNotificationAds(creatives.notification_ads);
  database::SaveCreativeInlineContentAds(creatives.inline_content_ads);
  database::SaveCreativeNewTabPageAds(creatives.new_tab_page_ads);
  database::SaveCreativePromotedContentAds(creatives.promoted_content_ads);
  database::SaveConversions(creatives.conversions);
}

void ResetCatalog() {
  AdsClientHelper::Get()->ClearPref(prefs::kCatalogId);
  AdsClientHelper::Get()->ClearPref(prefs::kCatalogVersion);
  AdsClientHelper::Get()->ClearPref(prefs::kCatalogPing);
  AdsClientHelper::Get()->ClearPref(prefs::kCatalogLastUpdated);
}

std::string GetCatalogId() {
  return AdsClientHelper::Get()->GetStringPref(prefs::kCatalogId);
}

void SetCatalogId(const std::string& id) {
  AdsClientHelper::Get()->SetStringPref(prefs::kCatalogId, id);
}

int GetCatalogVersion() {
  return AdsClientHelper::Get()->GetIntegerPref(prefs::kCatalogVersion);
}

void SetCatalogVersion(const int version) {
  AdsClientHelper::Get()->SetIntegerPref(prefs::kCatalogVersion, version);
}

base::TimeDelta GetCatalogPing() {
  const int64_t ping =
      AdsClientHelper::Get()->GetInt64Pref(prefs::kCatalogPing);
  return base::Milliseconds(ping);
}

void SetCatalogPing(const base::TimeDelta ping) {
  AdsClientHelper::Get()->SetInt64Pref(prefs::kCatalogPing,
                                       ping.InMilliseconds());
}

base::Time GetCatalogLastUpdated() {
  return AdsClientHelper::Get()->GetTimePref(prefs::kCatalogLastUpdated);
}

void SetCatalogLastUpdated(const base::Time last_updated_at) {
  AdsClientHelper::Get()->SetTimePref(prefs::kCatalogLastUpdated,
                                      last_updated_at);
}

bool DoesCatalogExist() {
  return GetCatalogVersion() > 0;
}

bool HasCatalogChanged(const std::string& catalog_id) {
  return catalog_id != GetCatalogId();
}

bool HasCatalogExpired() {
  if (base::Time::Now() < GetCatalogLastUpdated() + kCatalogLifespan) {
    return false;
  }

  return true;
}

}  // namespace ads
