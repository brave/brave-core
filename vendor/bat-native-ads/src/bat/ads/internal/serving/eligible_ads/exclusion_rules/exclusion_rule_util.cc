/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_util.h"

#include <algorithm>

#include "base/time/time.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"

namespace ads {

bool DoesRespectCampaignCap(const CreativeAdInfo& creative_ad,
                            const AdEventList& ad_events,
                            const ConfirmationType& confirmation_type,
                            const base::TimeDelta time_constraint,
                            const int cap) {
  const int count = std::count_if(
      ad_events.cbegin(), ad_events.cend(),
      [&creative_ad, &confirmation_type,
       time_constraint](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == confirmation_type &&
               ad_event.campaign_id == creative_ad.campaign_id &&
               base::Time::Now() - ad_event.created_at < time_constraint;
      });

  return count < cap;
}

bool DoesRespectCreativeSetCap(const CreativeAdInfo& creative_ad,
                               const AdEventList& ad_events,
                               const ConfirmationType& confirmation_type,
                               const base::TimeDelta time_constraint,
                               const int cap) {
  const int count = std::count_if(
      ad_events.cbegin(), ad_events.cend(),
      [&creative_ad, &confirmation_type,
       time_constraint](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == confirmation_type &&
               ad_event.creative_set_id == creative_ad.creative_set_id &&
               base::Time::Now() - ad_event.created_at < time_constraint;
      });

  return count < cap;
}

bool DoesRespectCreativeCap(const CreativeAdInfo& creative_ad,
                            const AdEventList& ad_events,
                            const ConfirmationType& confirmation_type,
                            const base::TimeDelta time_constraint,
                            const int cap) {
  const int count = std::count_if(
      ad_events.cbegin(), ad_events.cend(),
      [&creative_ad, &confirmation_type,
       time_constraint](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == confirmation_type &&
               ad_event.creative_instance_id ==
                   creative_ad.creative_instance_id &&
               base::Time::Now() - ad_event.created_at < time_constraint;
      });

  return count < cap;
}

}  // namespace ads
