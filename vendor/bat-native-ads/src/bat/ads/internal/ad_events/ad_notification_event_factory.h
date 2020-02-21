/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_FACTORY_H_
#define BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_FACTORY_H_

#include <memory>

#include "bat/ads/internal/ad_events/ad_event.h"
#include "bat/ads/mojom.h"

namespace ads {

class AdsImpl;
struct AdNotificationInfo;

class AdEventFactory {
 public:
  static std::unique_ptr<AdEvent<AdNotificationInfo>> Build(
      AdsImpl* ads,
      const AdNotificationEventType event_type);
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_FACTORY_H_
