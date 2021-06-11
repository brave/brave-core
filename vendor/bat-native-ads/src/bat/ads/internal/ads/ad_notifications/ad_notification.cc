/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_notifications/ad_notification.h"

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_factory.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/logging.h"

namespace ads {

AdNotification::AdNotification() = default;

AdNotification::~AdNotification() = default;

void AdNotification::AddObserver(AdNotificationObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void AdNotification::RemoveObserver(AdNotificationObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void AdNotification::FireEvent(const std::string& uuid,
                               const AdNotificationEventType event_type) {
  DCHECK(!uuid.empty());

  AdNotificationInfo ad;
  if (!AdNotifications::Get()->Get(uuid, &ad)) {
    BLOG(1,
         "Failed to fire ad notification event due to missing uuid " << uuid);
    NotifyAdNotificationEventFailed(uuid, event_type);
    return;
  }

  const auto ad_event = ad_notifications::AdEventFactory::Build(event_type);
  ad_event->FireEvent(ad);

  NotifyAdNotificationEvent(ad, event_type);
}

///////////////////////////////////////////////////////////////////////////////

void AdNotification::NotifyAdNotificationEvent(
    const AdNotificationInfo& ad,
    const AdNotificationEventType event_type) const {
  switch (event_type) {
    case AdNotificationEventType::kServed: {
      NotifyAdNotificationServed(ad);
      break;
    }

    case AdNotificationEventType::kViewed: {
      NotifyAdNotificationViewed(ad);
      break;
    }

    case AdNotificationEventType::kClicked: {
      NotifyAdNotificationClicked(ad);
      break;
    }

    case AdNotificationEventType::kDismissed: {
      NotifyAdNotificationDismissed(ad);
      break;
    }

    case AdNotificationEventType::kTimedOut: {
      NotifyAdNotificationTimedOut(ad);
      break;
    }
  }
}

void AdNotification::NotifyAdNotificationServed(
    const AdNotificationInfo& ad) const {
  for (AdNotificationObserver& observer : observers_) {
    observer.OnAdNotificationServed(ad);
  }
}

void AdNotification::NotifyAdNotificationViewed(
    const AdNotificationInfo& ad) const {
  for (AdNotificationObserver& observer : observers_) {
    observer.OnAdNotificationViewed(ad);
  }
}

void AdNotification::NotifyAdNotificationClicked(
    const AdNotificationInfo& ad) const {
  for (AdNotificationObserver& observer : observers_) {
    observer.OnAdNotificationClicked(ad);
  }
}

void AdNotification::NotifyAdNotificationDismissed(
    const AdNotificationInfo& ad) const {
  for (AdNotificationObserver& observer : observers_) {
    observer.OnAdNotificationDismissed(ad);
  }
}

void AdNotification::NotifyAdNotificationTimedOut(
    const AdNotificationInfo& ad) const {
  for (AdNotificationObserver& observer : observers_) {
    observer.OnAdNotificationTimedOut(ad);
  }
}

void AdNotification::NotifyAdNotificationEventFailed(
    const std::string& uuid,
    const AdNotificationEventType event_type) const {
  for (AdNotificationObserver& observer : observers_) {
    observer.OnAdNotificationEventFailed(uuid, event_type);
  }
}

}  // namespace ads
