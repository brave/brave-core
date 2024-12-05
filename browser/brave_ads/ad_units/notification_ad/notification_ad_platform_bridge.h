/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_AD_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_PLATFORM_BRIDGE_H_
#define BRAVE_BROWSER_BRAVE_ADS_AD_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_PLATFORM_BRIDGE_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/browser/ui/brave_ads/notification_ad.h"
#include "chrome/browser/profiles/profile.h"

namespace brave_ads {

class NotificationAdPlatformBridge {
 public:
  explicit NotificationAdPlatformBridge(Profile& profile);

  NotificationAdPlatformBridge(const NotificationAdPlatformBridge&) = delete;
  NotificationAdPlatformBridge& operator=(const NotificationAdPlatformBridge&) =
      delete;

  ~NotificationAdPlatformBridge();

  void ShowNotificationAd(NotificationAd notification_ad);
  void CloseNotificationAd(const std::string& notification_id);

 private:
  const raw_ref<Profile> profile_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_AD_UNITS_NOTIFICATION_AD_NOTIFICATION_AD_PLATFORM_BRIDGE_H_
