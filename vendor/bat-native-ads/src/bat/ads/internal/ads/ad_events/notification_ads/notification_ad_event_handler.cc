/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_handler.h"

#include "absl/types/optional.h"
#include "base/check.h"
#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_factory.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_manager.h"
#include "bat/ads/notification_ad_info.h"

namespace ads::notification_ads {

EventHandler::EventHandler() = default;

EventHandler::~EventHandler() = default;

void EventHandler::AddObserver(EventHandlerObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void EventHandler::RemoveObserver(EventHandlerObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void EventHandler::FireEvent(const std::string& placement_id,
                             const mojom::NotificationAdEventType event_type) {
  DCHECK(!placement_id.empty());
  DCHECK(mojom::IsKnownEnumValue(event_type));

  const absl::optional<NotificationAdInfo> ad =
      NotificationAdManager::GetInstance()->MaybeGetForPlacementId(
          placement_id);
  if (!ad) {
    BLOG(1, "Failed to fire notification ad event due to missing placement id "
                << placement_id);
    return FailedToFireEvent(placement_id, event_type);
  }

  const auto ad_event = AdEventFactory::Build(event_type);
  ad_event->FireEvent(*ad);

  NotifyNotificationAdEvent(*ad, event_type);
}

///////////////////////////////////////////////////////////////////////////////

void EventHandler::FailedToFireEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  BLOG(1, "Failed to fire notification ad "
              << event_type << " event for placement id " << placement_id);

  NotifyNotificationAdEventFailed(placement_id, event_type);
}

void EventHandler::NotifyNotificationAdEvent(
    const NotificationAdInfo& ad,
    const mojom::NotificationAdEventType event_type) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  switch (event_type) {
    case mojom::NotificationAdEventType::kServed: {
      NotifyNotificationAdServed(ad);
      break;
    }

    case mojom::NotificationAdEventType::kViewed: {
      NotifyNotificationAdViewed(ad);
      break;
    }

    case mojom::NotificationAdEventType::kClicked: {
      NotifyNotificationAdClicked(ad);
      break;
    }

    case mojom::NotificationAdEventType::kDismissed: {
      NotifyNotificationAdDismissed(ad);
      break;
    }

    case mojom::NotificationAdEventType::kTimedOut: {
      NotifyNotificationAdTimedOut(ad);
      break;
    }
  }
}

void EventHandler::NotifyNotificationAdServed(
    const NotificationAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnNotificationAdServed(ad);
  }
}

void EventHandler::NotifyNotificationAdViewed(
    const NotificationAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnNotificationAdViewed(ad);
  }
}

void EventHandler::NotifyNotificationAdClicked(
    const NotificationAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnNotificationAdClicked(ad);
  }
}

void EventHandler::NotifyNotificationAdDismissed(
    const NotificationAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnNotificationAdDismissed(ad);
  }
}

void EventHandler::NotifyNotificationAdTimedOut(
    const NotificationAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnNotificationAdTimedOut(ad);
  }
}

void EventHandler::NotifyNotificationAdEventFailed(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  for (EventHandlerObserver& observer : observers_) {
    observer.OnNotificationAdEventFailed(placement_id, event_type);
  }
}

}  // namespace ads::notification_ads
