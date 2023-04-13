/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_H_

#include <string>

#include "base/observer_list.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/notification_ads/notification_ad_event_handler_observer.h"

namespace brave_ads {

struct NotificationAdInfo;

namespace notification_ads {

class EventHandler final : public EventHandlerObserver {
 public:
  EventHandler();

  EventHandler(const EventHandler&) = delete;
  EventHandler& operator=(const EventHandler&) = delete;

  EventHandler(EventHandler&&) noexcept = delete;
  EventHandler& operator=(EventHandler&&) noexcept = delete;

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
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_H_
