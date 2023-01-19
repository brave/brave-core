/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/creatives/creative_ad_unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ads_database_table.h"

namespace ads {

void SaveCreativeAds(const CreativeNotificationAdList& creative_ads) {
  database::table::CreativeNotificationAds database_table;
  database_table.Save(
      creative_ads, base::BindOnce([](const bool success) { CHECK(success); }));
}

CreativeNotificationAdList BuildCreativeNotificationAds(const int count) {
  CreativeNotificationAdList creative_ads;

  for (int i = 0; i < count; i++) {
    const CreativeNotificationAdInfo creative_ad =
        BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

CreativeNotificationAdInfo BuildCreativeNotificationAd(
    const bool should_use_random_guids) {
  const CreativeAdInfo creative_ad = BuildCreativeAd(should_use_random_guids);
  CreativeNotificationAdInfo creative_notification_ad(creative_ad);

  creative_notification_ad.title = "Test Ad Title";
  creative_notification_ad.body = "Test Ad Body";

  return creative_notification_ad;
}

}  // namespace ads
