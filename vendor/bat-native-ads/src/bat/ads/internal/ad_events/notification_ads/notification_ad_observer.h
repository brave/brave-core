/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

struct NotificationAdInfo;

class NotificationAdObserver : public base::CheckedObserver {
 public:
  // Invoked when a notification ad is served
  virtual void OnNotificationAdServed(const NotificationAdInfo& ad) {}

  // Invoked when a notification ad is viewed
  virtual void OnNotificationAdViewed(const NotificationAdInfo& ad) {}

  // Invoked when a notification ad is clicked
  virtual void OnNotificationAdClicked(const NotificationAdInfo& ad) {}

  // Invoked when a notification ad is dismissed
  virtual void OnNotificationAdDismissed(const NotificationAdInfo& ad) {}

  // Invoked when a notification ad times out
  virtual void OnNotificationAdTimedOut(const NotificationAdInfo& ad) {}

  // Invoked when a notification ad event fails
  virtual void OnNotificationAdEventFailed(
      const std::string& placement_id,
      const mojom::NotificationAdEventType event_type) {}

 protected:
  ~NotificationAdObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_OBSERVER_H_
