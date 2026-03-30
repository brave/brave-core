/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/notification_ad_popup_collection.h"

#include <map>
#include <string>

#include "base/check.h"
#include "base/containers/map_util.h"
#include "base/no_destructor.h"
#include "brave/browser/ui/views/brave_ads/notification_ad_popup.h"

namespace brave_ads {

namespace {

std::map<std::string, NotificationAdPopup* /*NOT OWNED*/>&
GetNotificationAdPopups() {
  static base::NoDestructor<
      std::map<std::string, NotificationAdPopup* /*NOT OWNED*/>>
      popups;
  return *popups;
}

}  // namespace

NotificationAdPopupCollection::NotificationAdPopupCollection() = default;

NotificationAdPopupCollection::~NotificationAdPopupCollection() = default;

// static
void NotificationAdPopupCollection::Add(NotificationAdPopup* popup,
                                        const std::string& notification_id) {
  CHECK(!notification_id.empty());
  CHECK(!GetNotificationAdPopups().contains(notification_id));
  GetNotificationAdPopups()[notification_id] = popup;
}

// static
NotificationAdPopup* NotificationAdPopupCollection::Get(
    const std::string& notification_id) {
  CHECK(!notification_id.empty());

  NotificationAdPopup* popup =
      base::FindPtrOrNull(GetNotificationAdPopups(), notification_id);
  return popup;
}

// static
void NotificationAdPopupCollection::Remove(const std::string& notification_id) {
  CHECK(!notification_id.empty());

  // Note: The pointed-to NotificationAdPopup members are deallocated by their
  // containing Widgets
  GetNotificationAdPopups().erase(notification_id);
}

}  // namespace brave_ads
