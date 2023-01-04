/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_VIEW_FACTORY_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_VIEW_FACTORY_H_

#include <memory>

namespace brave_ads {

class NotificationAd;
class NotificationAdView;

// Creates a NotificationAdView depending on the notification type. A
// notification is top level if it needs to be rendered outside the browser
// window
class NotificationAdViewFactory {
 public:
  static std::unique_ptr<NotificationAdView> Create(
      const NotificationAd& notification_ad);
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_VIEW_FACTORY_H_
