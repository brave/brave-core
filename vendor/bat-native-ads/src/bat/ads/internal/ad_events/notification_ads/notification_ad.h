/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_H_

#include <string>

#include "base/observer_list.h"
#include "bat/ads/internal/ad_events/notification_ads/notification_ad_observer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

struct NotificationAdInfo;

class NotificationAd final : public NotificationAdObserver {
 public:
  NotificationAd();
  ~NotificationAd() override;
  NotificationAd(const NotificationAd& info) = delete;
  NotificationAd& operator=(const NotificationAd& info) = delete;

  void AddObserver(NotificationAdObserver* observer);
  void RemoveObserver(NotificationAdObserver* observer);

  void FireEvent(const std::string& placement_id,
                 const mojom::NotificationAdEventType event_type);

 private:
  void NotifyNotificationAdEvent(
      const NotificationAdInfo& ad,
      const mojom::NotificationAdEventType event_type) const;
  void NotifyNotificationAdServed(const NotificationAdInfo& ad) const;
  void NotifyNotificationAdViewed(const NotificationAdInfo& ad) const;
  void NotifyNotificationAdClicked(const NotificationAdInfo& ad) const;
  void NotifyNotificationAdDismissed(const NotificationAdInfo& ad) const;
  void NotifyNotificationAdTimedOut(const NotificationAdInfo& ad) const;

  void NotifyNotificationAdEventFailed(
      const std::string& placement_id,
      const mojom::NotificationAdEventType event_type) const;

  base::ObserverList<NotificationAdObserver> observers_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_H_
