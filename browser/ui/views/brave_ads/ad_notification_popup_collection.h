/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_AD_NOTIFICATION_POPUP_COLLECTION_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_AD_NOTIFICATION_POPUP_COLLECTION_H_

#include <string>

namespace brave_ads {

class AdNotificationPopup;

class AdNotificationPopupCollection final {
 public:
  AdNotificationPopupCollection();
  AdNotificationPopupCollection(const AdNotificationPopupCollection&) = delete;
  AdNotificationPopupCollection& operator=(
      const AdNotificationPopupCollection&) = delete;
  ~AdNotificationPopupCollection();

  static void Add(AdNotificationPopup* popup,
                  const std::string& notification_id);
  static AdNotificationPopup* Get(const std::string& notification_id);
  static void Remove(const std::string& notification_id);
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_AD_NOTIFICATION_POPUP_COLLECTION_H_
