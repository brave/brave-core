/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"

namespace brave_ads {

namespace {

bool ShouldAlwaysRespectCap(const base::TimeDelta time_constraint,
                            const size_t cap) {
  return time_constraint.is_zero() || cap == 0;
}

}  // namespace

bool DoesRespectCampaignCap(const CreativeAdInfo& creative_ad,
                            const AdEventList& ad_events,
                            const ConfirmationType confirmation_type,
                            const base::TimeDelta time_constraint,
                            const size_t cap) {
  if (ShouldAlwaysRespectCap(time_constraint, cap)) {
    return true;
  }

  const base::Time now = base::Time::Now();

  const size_t count = count_if_until(
      ad_events,
      [&creative_ad, &confirmation_type, now,
       time_constraint](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == confirmation_type &&
               ad_event.campaign_id == creative_ad.campaign_id &&
               now - ad_event.created_at < time_constraint;
      },
      cap);

  return count < cap;
}

bool DoesRespectCreativeSetCap(const CreativeAdInfo& creative_ad,
                               const AdEventList& ad_events,
                               const ConfirmationType confirmation_type,
                               const base::TimeDelta time_constraint,
                               const size_t cap) {
  if (ShouldAlwaysRespectCap(time_constraint, cap)) {
    return true;
  }

  const base::Time now = base::Time::Now();

  const size_t count = count_if_until(
      ad_events,
      [&creative_ad, &confirmation_type, now,
       time_constraint](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == confirmation_type &&
               ad_event.creative_set_id == creative_ad.creative_set_id &&
               now - ad_event.created_at < time_constraint;
      },
      cap);

  return count < cap;
}

bool DoesRespectCreativeCap(const CreativeAdInfo& creative_ad,
                            const AdEventList& ad_events,
                            const ConfirmationType confirmation_type,
                            const base::TimeDelta time_constraint,
                            const size_t cap) {
  if (ShouldAlwaysRespectCap(time_constraint, cap)) {
    return true;
  }

  const base::Time now = base::Time::Now();

  const size_t count = count_if_until(
      ad_events,
      [&creative_ad, &confirmation_type, now,
       time_constraint](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == confirmation_type &&
               ad_event.creative_instance_id ==
                   creative_ad.creative_instance_id &&
               now - ad_event.created_at < time_constraint;
      },
      cap);

  return count < cap;
}

}  // namespace brave_ads
