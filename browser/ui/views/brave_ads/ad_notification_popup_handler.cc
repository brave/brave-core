/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_ads/ad_notification_popup_handler.h"

#include "brave/browser/ui/brave_ads/ad_notification.h"
#include "brave/browser/ui/brave_ads/ad_notification_delegate.h"
#include "brave/browser/ui/views/brave_ads/ad_notification_popup.h"
#include "ui/gfx/geometry/vector2d.h"

namespace brave_ads {

namespace {

std::map<std::string, AdNotificationPopup* /* NOT OWNED */>
    g_ad_notification_popups;

}  // namespace

AdNotificationPopupHandler::AdNotificationPopupHandler() = default;

AdNotificationPopupHandler::~AdNotificationPopupHandler() = default;

// static
void AdNotificationPopupHandler::Show(Profile* profile,
                                      const AdNotification& ad_notification) {
  DCHECK(profile);

  const std::string& id = ad_notification.id();
  DCHECK(!id.empty());

  AdNotificationPopup* popup =
      new AdNotificationPopup(profile, ad_notification);
  DCHECK(!g_ad_notification_popups[id]);
  g_ad_notification_popups[id] = popup;

  AdNotificationDelegate* delegate = ad_notification.delegate();
  if (delegate) {
    delegate->OnShow();
  }
}

// static
void AdNotificationPopupHandler::Close(const std::string& notification_id,
                                       bool by_user) {
  DCHECK(!notification_id.empty());

  if (!g_ad_notification_popups[notification_id]) {
    return;
  }

  AdNotificationPopup* popup = g_ad_notification_popups[notification_id];
  DCHECK(popup);

  // Note: The pointed-to AdNotificationPopup members are deallocated by their
  // containing Widgets
  g_ad_notification_popups.erase(notification_id);

  const AdNotification ad_notification = popup->GetAdNotification();
  AdNotificationDelegate* delegate = ad_notification.delegate();
  if (delegate) {
    delegate->OnClose(by_user);
  }

  popup->ClosePopup();
}

// static
void AdNotificationPopupHandler::Move(const std::string& notification_id,
                                      const gfx::Vector2d& distance) {
  DCHECK(!notification_id.empty());

  if (!g_ad_notification_popups[notification_id]) {
    return;
  }

  AdNotificationPopup* popup = g_ad_notification_popups[notification_id];
  DCHECK(popup);

  popup->MovePopup(distance);
}

}  // namespace brave_ads
