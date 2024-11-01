/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ADS_NOTIFICATION_AD_POPUP_HANDLER_H_
#define BRAVE_BROWSER_UI_BRAVE_ADS_NOTIFICATION_AD_POPUP_HANDLER_H_

#include <string>

#include "ui/gfx/native_widget_types.h"

class Profile;

namespace gfx {
class Vector2d;
}  // namespace gfx

namespace brave_ads {

class NotificationAd;

class NotificationAdPopupHandler final {
 public:
  NotificationAdPopupHandler();

  NotificationAdPopupHandler(const NotificationAdPopupHandler&) = delete;
  NotificationAdPopupHandler& operator=(const NotificationAdPopupHandler&) =
      delete;

  NotificationAdPopupHandler(NotificationAdPopupHandler&&) noexcept = delete;
  NotificationAdPopupHandler& operator=(NotificationAdPopupHandler&&) noexcept =
      delete;

  ~NotificationAdPopupHandler();

  // Show the notification popup view for the given `profile` and
  // `notification_ad`.
  static void Show(Profile& profile,
                   const NotificationAd& notification_ad,
                   gfx::NativeWindow browser_native_window,
                   gfx::NativeView browser_native_view);

  // Close the notification popup view for the given `notification_id`.
  // `by_user` is `true` if the notification popup was closed by the user,
  // otherwise `false`.
  static void Close(const std::string& notification_id, bool by_user);
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_BRAVE_ADS_NOTIFICATION_AD_POPUP_HANDLER_H_
