/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/notification_ad/notification_ad_handler_util.h"

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_manager.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

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
         (BrowserManager::GetInstance().IsInForeground() ||
          CanShowNotificationAdsWhileBrowserIsBackgrounded()) &&
         GetMaximumNotificationAdsPerHour() > 0;
}

}  // namespace brave_ads
