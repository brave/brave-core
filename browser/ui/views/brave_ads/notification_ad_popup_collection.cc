/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/notification_ad_popup_collection.h"

#include <map>

#include "brave/browser/ui/views/brave_ads/notification_ad_popup.h"

namespace brave_ads {

namespace {

std::map<std::string, NotificationAdPopup* /*NOT OWNED*/>
    g_notification_ad_popups;

}  // namespace

NotificationAdPopupCollection::NotificationAdPopupCollection() = default;

NotificationAdPopupCollection::~NotificationAdPopupCollection() = default;

// static
void NotificationAdPopupCollection::Add(NotificationAdPopup* popup,
                                        const std::string& notification_id) {
  CHECK(!notification_id.empty());
  CHECK_EQ(g_notification_ad_popups.count(notification_id), 0u);
  g_notification_ad_popups[notification_id] = popup;
}

// static
NotificationAdPopup* NotificationAdPopupCollection::Get(
    const std::string& notification_id) {
  CHECK(!notification_id.empty());
  if (g_notification_ad_popups.count(notification_id) == 0) {
    return nullptr;
  }

  NotificationAdPopup* popup = g_notification_ad_popups[notification_id];
  CHECK(popup);

  return popup;
}

// static
void NotificationAdPopupCollection::Remove(const std::string& notification_id) {
  CHECK(!notification_id.empty());
  if (g_notification_ad_popups.count(notification_id) == 0) {
    return;
  }

  // Note: The pointed-to NotificationAdPopup members are deallocated by their
  // containing Widgets
  g_notification_ad_popups.erase(notification_id);
}

}  // namespace brave_ads
