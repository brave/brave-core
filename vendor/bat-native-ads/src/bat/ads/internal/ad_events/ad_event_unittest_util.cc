/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"

#include "base/guid.h"
#include "base/time/time.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"

namespace ads {

AdEventInfo GetAdEvent(const CreativeAdInfo& creative_ad,
                       const ConfirmationType confirmation_type,
                       const base::Time& created_at) {
  AdEventInfo ad_event;
  ad_event.uuid = base::GenerateGUID();
  ad_event.confirmation_type = confirmation_type;
  ad_event.creative_instance_id = creative_ad.creative_instance_id;
  ad_event.advertiser_id = creative_ad.advertiser_id;
  ad_event.created_at = created_at;

  return ad_event;
}

}  // namespace ads
