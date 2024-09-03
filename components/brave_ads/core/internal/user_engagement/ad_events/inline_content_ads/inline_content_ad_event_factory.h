/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_FACTORY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_FACTORY_H_

#include <memory>

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace brave_ads {

struct InlineContentAdInfo;

class InlineContentAdEventFactory final {
 public:
  static std::unique_ptr<AdEventInterface<InlineContentAdInfo>> Build(
      mojom::InlineContentAdEventType mojom_ad_event_type);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_FACTORY_H_
