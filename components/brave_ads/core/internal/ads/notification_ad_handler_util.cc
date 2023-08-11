/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/notification_ad_handler_util.h"

#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/browser/browser_manager.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"

namespace brave_ads {

bool ShouldServe() {
  return UserHasOptedInToNotificationAds();
}

bool CanServeIfUserIsActive() {
  return !PlatformHelper::GetInstance().IsMobile();
}

bool CanServeAtRegularIntervals() {
  return PlatformHelper::GetInstance().IsMobile();
}

bool ShouldServeAtRegularIntervals() {
  return ShouldServe() &&
         (BrowserManager::GetInstance().IsBrowserInForeground() ||
          AdsClientHelper::GetInstance()
              ->CanShowNotificationAdsWhileBrowserIsBackgrounded()) &&
         GetMaximumNotificationAdsPerHour() > 0;
}

void ShowNotificationAd(const NotificationAdInfo& ad) {
  NotificationAdManager::GetInstance().Add(ad);
  AdsClientHelper::GetInstance()->ShowNotificationAd(ad);
}

void DismissNotificationAd(const std::string& placement_id) {
  NotificationAdManager::GetInstance().Remove(placement_id);
}

void CloseNotificationAd(const std::string& placement_id) {
  NotificationAdManager::GetInstance().Remove(placement_id);
  AdsClientHelper::GetInstance()->CloseNotificationAd(placement_id);
}

void NotificationAdTimedOut(const std::string& placement_id) {
  NotificationAdManager::GetInstance().Remove(placement_id);
}

}  // namespace brave_ads
