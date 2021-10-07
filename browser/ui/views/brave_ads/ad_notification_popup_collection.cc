/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/ad_notification_popup_collection.h"

#include <map>

#include "brave/browser/ui/views/brave_ads/ad_notification_popup.h"

namespace brave_ads {

namespace {

std::map<std::string, AdNotificationPopup* /* NOT OWNED */>
    g_ad_notification_popups;

}  // namespace

AdNotificationPopupCollection::AdNotificationPopupCollection() = default;

AdNotificationPopupCollection::~AdNotificationPopupCollection() = default;

// static
void AdNotificationPopupCollection::Add(AdNotificationPopup* popup,
                                        const std::string& notification_id) {
  DCHECK(!notification_id.empty());
  DCHECK_EQ(g_ad_notification_popups.count(notification_id), 0u);
  g_ad_notification_popups[notification_id] = popup;
}

// static
AdNotificationPopup* AdNotificationPopupCollection::Get(
    const std::string& notification_id) {
  DCHECK(!notification_id.empty());
  if (g_ad_notification_popups.count(notification_id) == 0) {
    return nullptr;
  }

  AdNotificationPopup* popup = g_ad_notification_popups[notification_id];
  DCHECK(popup);

  return popup;
}

// static
void AdNotificationPopupCollection::Remove(const std::string& notification_id) {
  DCHECK(!notification_id.empty());
  if (g_ad_notification_popups.count(notification_id) == 0) {
    return;
  }

  // Note: The pointed-to AdNotificationPopup members are deallocated by their
  // containing Widgets
  g_ad_notification_popups.erase(notification_id);
}

}  // namespace brave_ads
