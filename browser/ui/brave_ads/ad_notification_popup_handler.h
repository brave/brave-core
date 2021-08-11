/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_POPUP_HANDLER_H_
#define BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_POPUP_HANDLER_H_

#include <string>

class Profile;

namespace brave_ads {

class AdNotification;

class AdNotificationPopupHandler final {
 public:
  AdNotificationPopupHandler();
  AdNotificationPopupHandler(const AdNotificationPopupHandler&) = delete;
  AdNotificationPopupHandler& operator=(const AdNotificationPopupHandler&) =
      delete;
  ~AdNotificationPopupHandler();

  // Show the notification popup view for the given |profile| and
  // |ad_notification|.
  static void Show(Profile* profile, const AdNotification& ad_notification);

  // Close the notification popup view for the given |notification_id|.
  static void Close(const std::string& notification_id);
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_POPUP_HANDLER_H_
