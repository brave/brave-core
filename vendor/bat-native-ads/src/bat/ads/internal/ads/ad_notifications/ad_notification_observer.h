/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_NOTIFICATIONS_AD_NOTIFICATION_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_NOTIFICATIONS_AD_NOTIFICATION_OBSERVER_H_

#include <string>

#include "base/observer_list.h"
#include "bat/ads/mojom.h"

namespace ads {

struct AdNotificationInfo;

class AdNotificationObserver : public base::CheckedObserver {
 public:
  // Invoked when an ad notification is served
  virtual void OnAdNotificationServed(const AdNotificationInfo& ad) {}

  // Invoked when an ad notification is viewed
  virtual void OnAdNotificationViewed(const AdNotificationInfo& ad) {}

  // Invoked when an ad notification is clicked
  virtual void OnAdNotificationClicked(const AdNotificationInfo& ad) {}

  // Invoked when an ad notification is dismissed
  virtual void OnAdNotificationDismissed(const AdNotificationInfo& ad) {}

  // Invoked when an ad notification times out
  virtual void OnAdNotificationTimedOut(const AdNotificationInfo& ad) {}

  // Invoked when an ad notification event fails
  virtual void OnAdNotificationEventFailed(
      const std::string& uuid,
      const AdNotificationEventType event_type) {}

 protected:
  ~AdNotificationObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_NOTIFICATIONS_AD_NOTIFICATION_OBSERVER_H_
