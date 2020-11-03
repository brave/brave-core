/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_EVENTS_AD_NOTIFICATIONS_AD_NOTIFICATION_EVENT_FACTORY_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_EVENTS_AD_NOTIFICATIONS_AD_NOTIFICATION_EVENT_FACTORY_H_  // NOLINT

#include <memory>

#include "bat/ads/internal/ad_events/ad_event.h"
#include "bat/ads/mojom.h"

namespace ads {

class AdsImpl;
struct AdNotificationInfo;

namespace ad_notifications {

class AdEventFactory {
 public:
  static std::unique_ptr<AdEvent<AdNotificationInfo>> Build(
      AdsImpl* ads,
      const AdNotificationEventType event_type);
};

}  // namespace ad_notifications
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_EVENTS_AD_NOTIFICATIONS_AD_NOTIFICATION_EVENT_FACTORY_H_  // NOLINT
