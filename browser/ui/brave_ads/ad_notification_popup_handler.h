/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_POPUP_HANDLER_H_
#define BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_POPUP_HANDLER_H_

#include <string>

#include "ui/gfx/native_widget_types.h"

class Profile;

namespace gfx {
class Vector2d;
}

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
  static void Show(Profile* profile,
                   const AdNotification& ad_notification,
                   gfx::NativeWindow browser_native_window,
                   gfx::NativeView browser_native_view);

  // Close the notification popup view for the given |notification_id|.
  // |by_user| is true if the notification popup was closed by the user,
  // otherwise false.
  static void Close(const std::string& notification_id, bool by_user);

  // Move the notification popup view for the given |notification_id| by a
  // distance.
  static void Move(const std::string& notification_id,
                   const gfx::Vector2d& distance);
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_POPUP_HANDLER_H_
