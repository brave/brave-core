/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_ads/ad_notification_popup_handler.h"

#include "brave/browser/ui/views/brave_ads/ad_notification_popup.h"

namespace brave_ads {

AdNotificationPopupHandler::AdNotificationPopupHandler() = default;

AdNotificationPopupHandler::~AdNotificationPopupHandler() = default;

// static
void AdNotificationPopupHandler::Show(Profile* profile,
                                      const AdNotification& ad_notification) {
  AdNotificationPopup::Show(profile, ad_notification);
}

// static
void AdNotificationPopupHandler::Close(const std::string& notification_id) {
  AdNotificationPopup::Close(notification_id, /* by_user */ false);
}

}  // namespace brave_ads
