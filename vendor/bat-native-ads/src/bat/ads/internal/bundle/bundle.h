/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_BUNDLE_BUNDLE_H_
#define BAT_ADS_INTERNAL_BUNDLE_BUNDLE_H_

#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/conversions/conversion_info.h"

namespace ads {

class Catalog;
struct BundleState;

class Bundle {
 public:
  Bundle();

  ~Bundle();

  void BuildFromCatalog(
      const Catalog& catalog);

 private:
  BundleState FromCatalog(
      const Catalog& catalog) const;

  void DeleteDatabaseTables();

  void DeleteCreativeAdNotifications();
  void DeleteCreativeNewTabPageAds();
  void DeleteCampaigns();
  void DeleteSegments();
  void DeleteCreativeAds();
  void DeleteDayparts();
  void DeleteGeoTargets();

  void SaveCreativeAdNotifications(
      const CreativeAdNotificationList& creative_ad_notifications);

  void SaveCreativeNewTabPageAds(
      const CreativeNewTabPageAdList& creative_new_tab_page_ads);

  void PurgeExpiredConversions();
  void SaveConversions(
      const ConversionList& conversions);
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_BUNDLE_BUNDLE_H_
