/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/notification_ad/notification_ad_test_util.h"

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"

namespace brave_ads::test {

NotificationAdInfo BuildNotificationAd(
    const bool should_generate_random_uuids) {
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(should_generate_random_uuids);
  return BuildNotificationAd(creative_ad);
}

NotificationAdInfo BuildAndSaveNotificationAd(
    const bool should_generate_random_uuids) {
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(should_generate_random_uuids);
  database::SaveCreativeNotificationAds({creative_ad});
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  NotificationAdManager::GetInstance().Add(ad);
  return ad;
}

}  // namespace brave_ads::test
