/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"

#include <cstdint>

#include "base/guid.h"
#include "base/time/time.h"

namespace ads {

AdEventInfo GenerateAdEvent(const AdType type,
                            const CreativeAdInfo& ad,
                            const ConfirmationType& confirmation_type) {
  AdEventInfo ad_event;

  ad_event.uuid = base::GenerateGUID();
  ad_event.type = type;
  ad_event.confirmation_type = confirmation_type;
  ad_event.campaign_id = ad.campaign_id;
  ad_event.creative_set_id = ad.creative_set_id;
  ad_event.creative_instance_id = ad.creative_instance_id;
  ad_event.advertiser_id = ad.advertiser_id;
  ad_event.timestamp = static_cast<int64_t>(base::Time::Now().ToDoubleT());

  return ad_event;
}

AdEventInfo GenerateAdEvent(const AdType type,
                            const AdInfo& ad,
                            const ConfirmationType& confirmation_type) {
  AdEventInfo ad_event;

  ad_event.uuid = ad.uuid;
  ad_event.type = type;
  ad_event.confirmation_type = confirmation_type;
  ad_event.campaign_id = ad.campaign_id;
  ad_event.creative_set_id = ad.creative_set_id;
  ad_event.creative_instance_id = ad.creative_instance_id;
  ad_event.advertiser_id = ad.advertiser_id;
  ad_event.timestamp = static_cast<int64_t>(base::Time::Now().ToDoubleT());

  return ad_event;
}

}  // namespace ads
