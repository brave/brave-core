/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_event_util.h"

#include "bat/ads/ad_info.h"

namespace ads {

bool HasFiredAdViewedEvent(const AdInfo& ad, const AdEventList& ad_events) {
  const auto iter = std::find_if(
      ad_events.begin(), ad_events.end(), [&ad](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == ConfirmationType::kViewed &&
               ad_event.uuid == ad.uuid;
      });

  if (iter == ad_events.end()) {
    return false;
  }

  return true;
}

}  // namespace ads
