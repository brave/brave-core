/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_server/catalog/catalog_util.h"

#include "base/time/time.h"
#include "bat/ads/internal/account/deposits/deposits_database_util.h"
#include "bat/ads/internal/ad_server/catalog/catalog.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/conversions/conversions_database_util.h"
#include "bat/ads/internal/creatives/ad_notifications/creative_ad_notifications_database_util.h"
#include "bat/ads/internal/creatives/campaigns_database_util.h"
#include "bat/ads/internal/creatives/creative_ads_database_util.h"
#include "bat/ads/internal/creatives/creatives_builder.h"
#include "bat/ads/internal/creatives/creatives_info.h"
#include "bat/ads/internal/creatives/dayparts_database_util.h"
#include "bat/ads/internal/creatives/geo_targets_database_util.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_util.h"
#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_util.h"
#include "bat/ads/internal/creatives/segments_database_util.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {

constexpr int kCatalogLifespanInDays = 1;

void Delete() {
  database::DeleteCampaigns();
  database::DeleteCreativeAdNotifications();
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

void SaveCatalog(const Catalog& catalog) {
  Delete();

  PurgeExpired();

  AdsClientHelper::Get()->SetStringPref(prefs::kCatalogId, catalog.GetId());

  AdsClientHelper::Get()->SetIntegerPref(prefs::kCatalogVersion,
                                         catalog.GetVersion());

  AdsClientHelper::Get()->SetInt64Pref(prefs::kCatalogPing, catalog.GetPing());

  const CreativesInfo creatives = BuildCreatives(catalog);
  database::SaveCreativeAdNotifications(creatives.ad_notifications);
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

bool DoesCatalogExist() {
  return AdsClientHelper::Get()->GetIntegerPref(prefs::kCatalogVersion) > 0;
}

bool HasCatalogExpired() {
  const base::Time now = base::Time::Now();

  const double catalog_last_updated_timestamp =
      AdsClientHelper::Get()->GetDoublePref(prefs::kCatalogLastUpdated);

  const base::Time time =
      base::Time::FromDoubleT(catalog_last_updated_timestamp);

  if (now < time + base::Days(kCatalogLifespanInDays)) {
    return false;
  }

  return true;
}

}  // namespace ads
