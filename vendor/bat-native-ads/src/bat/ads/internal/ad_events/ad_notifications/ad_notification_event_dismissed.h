/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_NOTIFICATIONS_AD_NOTIFICATION_EVENT_DISMISSED_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_NOTIFICATIONS_AD_NOTIFICATION_EVENT_DISMISSED_H_

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/internal/ad_events/ad_event.h"

namespace ads {
namespace ad_notifications {

class AdEventDismissed : public AdEvent<AdNotificationInfo> {
 public:
  AdEventDismissed();

  ~AdEventDismissed() override;

  void FireEvent(const AdNotificationInfo& ad) override;
};

}  // namespace ad_notifications
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_NOTIFICATIONS_AD_NOTIFICATION_EVENT_DISMISSED_H_
