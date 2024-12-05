/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_POPUP_COLLECTION_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_POPUP_COLLECTION_H_

#include <string>

namespace brave_ads {

class NotificationAdPopup;

class NotificationAdPopupCollection final {
 public:
  NotificationAdPopupCollection();

  NotificationAdPopupCollection(const NotificationAdPopupCollection&) = delete;
  NotificationAdPopupCollection& operator=(
      const NotificationAdPopupCollection&) = delete;

  ~NotificationAdPopupCollection();

  static void Add(NotificationAdPopup* popup,
                  const std::string& notification_id);
  static NotificationAdPopup* Get(const std::string& notification_id);
  static void Remove(const std::string& notification_id);
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_POPUP_COLLECTION_H_
