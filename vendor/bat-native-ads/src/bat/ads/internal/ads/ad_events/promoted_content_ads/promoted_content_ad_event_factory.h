/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_FACTORY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_FACTORY_H_

#include <memory>

#include "bat/ads/internal/ads/ad_events/ad_event_interface.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"

namespace ads {

struct PromotedContentAdInfo;

namespace promoted_content_ads {

class AdEventFactory final {
 public:
  static std::unique_ptr<AdEventInterface<PromotedContentAdInfo>> Build(
      mojom::PromotedContentAdEventType event_type);
};

}  // namespace promoted_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_FACTORY_H_
