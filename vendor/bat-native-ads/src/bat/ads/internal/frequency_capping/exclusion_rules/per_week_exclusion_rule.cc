/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_week_exclusion_rule.h"

#include <algorithm>

#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/confirmation_type.h"

namespace ads {

PerWeekExclusionRule::PerWeekExclusionRule(const AdEventList& ad_events)
    : ad_events_(ad_events) {}

PerWeekExclusionRule::~PerWeekExclusionRule() = default;

std::string PerWeekExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool PerWeekExclusionRule::ShouldExclude(const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(ad_events_, creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s has exceeded the perWeek frequency cap",
        creative_ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string PerWeekExclusionRule::GetLastMessage() const {
  return last_message_;
}

bool PerWeekExclusionRule::DoesRespectCap(const AdEventList& ad_events,
                                          const CreativeAdInfo& creative_ad) {
  if (creative_ad.per_week == 0) {
    // Always respect cap if set to 0
    return true;
  }

  const base::Time now = base::Time::Now();

  const base::TimeDelta time_constraint = base::Days(7);

  const int count = std::count_if(
      ad_events.cbegin(), ad_events.cend(),
      [&now, &time_constraint, &creative_ad](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == ConfirmationType::kServed &&
               ad_event.creative_set_id == creative_ad.creative_set_id &&
               now - ad_event.created_at < time_constraint;
      });

  if (count >= creative_ad.per_week) {
    return false;
  }

  return true;
}

}  // namespace ads
