/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_NOTIFICATIONS_AD_NOTIFICATION_PLATFORM_BRIDGE_H_
#define BRAVE_BROWSER_BRAVE_ADS_NOTIFICATIONS_AD_NOTIFICATION_PLATFORM_BRIDGE_H_

#include <string>

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/brave_ads/ad_notification.h"

namespace brave_ads {

class AdNotificationPlatformBridge {
 public:
  explicit AdNotificationPlatformBridge(Profile* profile);
  ~AdNotificationPlatformBridge();

  void ShowAdNotification(AdNotification ad_notification);
  void CloseAdNotification(const std::string& notification_id);

 private:
  Profile* profile_ = nullptr;  // NOT OWNED

  AdNotificationPlatformBridge(const AdNotificationPlatformBridge&) = delete;
  AdNotificationPlatformBridge& operator=(const AdNotificationPlatformBridge&) =
      delete;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_NOTIFICATIONS_AD_NOTIFICATION_PLATFORM_BRIDGE_H_
