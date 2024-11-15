/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_ads/notification_ad_popup_handler.h"

#include "brave/browser/ui/brave_ads/notification_ad.h"
#include "brave/browser/ui/brave_ads/notification_ad_delegate.h"
#include "brave/browser/ui/views/brave_ads/notification_ad_popup.h"
#include "brave/browser/ui/views/brave_ads/notification_ad_popup_collection.h"

namespace brave_ads {

NotificationAdPopupHandler::NotificationAdPopupHandler() = default;

NotificationAdPopupHandler::~NotificationAdPopupHandler() = default;

// static
void NotificationAdPopupHandler::Show(Profile& profile,
                                      const NotificationAd& notification_ad,
                                      gfx::NativeWindow browser_native_window,
                                      gfx::NativeView browser_native_view) {
  const std::string& id = notification_ad.id();
  CHECK(!id.empty());

  NotificationAdPopup* popup = new NotificationAdPopup(
      profile, notification_ad, browser_native_window, browser_native_view);
  NotificationAdPopupCollection::Add(popup, id);

  NotificationAdDelegate* delegate = notification_ad.delegate();
  if (delegate) {
    delegate->OnShow();
  }
}

// static
void NotificationAdPopupHandler::Close(const std::string& notification_id,
                                       bool by_user) {
  CHECK(!notification_id.empty());

  NotificationAdPopup* popup =
      NotificationAdPopupCollection::Get(notification_id);
  if (!popup) {
    return;
  }
  // NotificationAdPopupCollection::Remove() is called later in the Widget
  // destroy event handler to handle the case when popup is closed externally
  // (for example, from the Windows taskbar).

  const NotificationAd notification_ad = popup->GetNotificationAd();
  NotificationAdDelegate* delegate = notification_ad.delegate();
  if (delegate) {
    delegate->OnClose(by_user);
  }

  popup->ClosePopup();
}

}  // namespace brave_ads
