/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/notification_ads/notification_ad.h"

#include "base/check.h"
#include "bat/ads/internal/ad_events/notification_ads/notification_ad_event_factory.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/creatives/notification_ads/notification_ads.h"
#include "bat/ads/notification_ad_info.h"

namespace ads {

NotificationAd::NotificationAd() = default;

NotificationAd::~NotificationAd() = default;

void NotificationAd::AddObserver(NotificationAdObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void NotificationAd::RemoveObserver(NotificationAdObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void NotificationAd::FireEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type) {
  DCHECK(!placement_id.empty());

  NotificationAdInfo ad;
  if (!NotificationAds::Get()->Get(placement_id, &ad)) {
    BLOG(1, "Failed to fire notification ad event due to missing placement id "
                << placement_id);
    NotifyNotificationAdEventFailed(placement_id, event_type);
    return;
  }

  const auto ad_event = notification_ads::AdEventFactory::Build(event_type);
  ad_event->FireEvent(ad);

  NotifyNotificationAdEvent(ad, event_type);
}

///////////////////////////////////////////////////////////////////////////////

void NotificationAd::NotifyNotificationAdEvent(
    const NotificationAdInfo& ad,
    const mojom::NotificationAdEventType event_type) const {
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

void NotificationAd::NotifyNotificationAdServed(
    const NotificationAdInfo& ad) const {
  for (NotificationAdObserver& observer : observers_) {
    observer.OnNotificationAdServed(ad);
  }
}

void NotificationAd::NotifyNotificationAdViewed(
    const NotificationAdInfo& ad) const {
  for (NotificationAdObserver& observer : observers_) {
    observer.OnNotificationAdViewed(ad);
  }
}

void NotificationAd::NotifyNotificationAdClicked(
    const NotificationAdInfo& ad) const {
  for (NotificationAdObserver& observer : observers_) {
    observer.OnNotificationAdClicked(ad);
  }
}

void NotificationAd::NotifyNotificationAdDismissed(
    const NotificationAdInfo& ad) const {
  for (NotificationAdObserver& observer : observers_) {
    observer.OnNotificationAdDismissed(ad);
  }
}

void NotificationAd::NotifyNotificationAdTimedOut(
    const NotificationAdInfo& ad) const {
  for (NotificationAdObserver& observer : observers_) {
    observer.OnNotificationAdTimedOut(ad);
  }
}

void NotificationAd::NotifyNotificationAdEventFailed(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type) const {
  for (NotificationAdObserver& observer : observers_) {
    observer.OnNotificationAdEventFailed(placement_id, event_type);
  }
}

}  // namespace ads
