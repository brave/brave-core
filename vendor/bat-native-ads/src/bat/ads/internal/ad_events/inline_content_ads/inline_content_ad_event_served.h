/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_SERVED_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_SERVED_H_

#include "bat/ads/internal/ad_events/ad_event.h"

namespace ads {

struct InlineContentAdInfo;

namespace inline_content_ads {

class AdEventServed : public AdEvent<InlineContentAdInfo> {
 public:
  AdEventServed();

  ~AdEventServed() override;

  void FireEvent(const InlineContentAdInfo& ad) override;
};

}  // namespace inline_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_SERVED_H_
