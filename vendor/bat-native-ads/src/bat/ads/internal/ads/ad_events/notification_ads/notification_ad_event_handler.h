/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_H_

#include <string>

#include "base/observer_list.h"
#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_handler_observer.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"

namespace ads {

struct NotificationAdInfo;

namespace notification_ads {

class EventHandler final : public EventHandlerObserver {
 public:
  EventHandler();

  EventHandler(const EventHandler& other) = delete;
  EventHandler& operator=(const EventHandler& other) = delete;

  EventHandler(EventHandler&& other) noexcept = delete;
  EventHandler& operator=(EventHandler&& other) noexcept = delete;

  ~EventHandler() override;

  void AddObserver(EventHandlerObserver* observer);
  void RemoveObserver(EventHandlerObserver* observer);

  void FireEvent(const std::string& placement_id,
                 mojom::NotificationAdEventType event_type);

 private:
  void FailedToFireEvent(const std::string& placement_id,
                         mojom::NotificationAdEventType event_type) const;

  void NotifyNotificationAdEvent(
      const NotificationAdInfo& ad,
      mojom::NotificationAdEventType event_type) const;
  void NotifyNotificationAdServed(const NotificationAdInfo& ad) const;
  void NotifyNotificationAdViewed(const NotificationAdInfo& ad) const;
  void NotifyNotificationAdClicked(const NotificationAdInfo& ad) const;
  void NotifyNotificationAdDismissed(const NotificationAdInfo& ad) const;
  void NotifyNotificationAdTimedOut(const NotificationAdInfo& ad) const;
  void NotifyNotificationAdEventFailed(
      const std::string& placement_id,
      mojom::NotificationAdEventType event_type) const;

  base::ObserverList<EventHandlerObserver> observers_;
};

}  // namespace notification_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_H_
