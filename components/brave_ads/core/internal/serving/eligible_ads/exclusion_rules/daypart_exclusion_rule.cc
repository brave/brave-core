/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/daypart_exclusion_rule.h"

#include <algorithm>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/calendar/calendar_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/daypart_exclusion_rule_util.h"

namespace brave_ads {

namespace {

bool DoesRespectCap(const CreativeAdInfo& creative_ad) {
  if (creative_ad.dayparts.empty()) {
    // Always respect cap if there are no dayparts specified.
    return true;
  }

  const base::Time now = base::Time::Now();
  const int day_of_week = DayOfWeek(now, /*is_local=*/true);
  const int minutes = LocalTimeInMinutesSinceMidnight(now);

  return std::ranges::any_of(
      creative_ad.dayparts,
      [day_of_week, minutes](const CreativeDaypartInfo& daypart) {
        return MatchDayOfWeek(daypart, day_of_week) &&
               MatchTimeSlot(daypart, minutes);
      });
}

}  // namespace

std::string DaypartExclusionRule::GetCacheKey(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool DaypartExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCap(creative_ad)) {
    BLOG(1, "creativeSetId "
                << creative_ad.creative_set_id
                << " excluded as not within a scheduled time slot");
    return false;
  }

  return true;
}

}  // namespace brave_ads
