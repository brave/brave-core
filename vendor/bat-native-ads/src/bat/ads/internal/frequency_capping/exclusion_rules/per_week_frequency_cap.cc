/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_week_frequency_cap.h"

#include <algorithm>

#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

namespace ads {

PerWeekFrequencyCap::PerWeekFrequencyCap(const AdEventList& ad_events)
    : ad_events_(ad_events) {}

PerWeekFrequencyCap::~PerWeekFrequencyCap() = default;

std::string PerWeekFrequencyCap::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool PerWeekFrequencyCap::ShouldExclude(const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(ad_events_, creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s has exceeded the perWeek frequency cap",
        creative_ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string PerWeekFrequencyCap::GetLastMessage() const {
  return last_message_;
}

bool PerWeekFrequencyCap::DoesRespectCap(const AdEventList& ad_events,
                                         const CreativeAdInfo& creative_ad) {
  if (creative_ad.per_week == 0) {
    // Always respect cap if set to 0
    return true;
  }

  const base::Time now = base::Time::Now();

  const base::TimeDelta time_constraint = base::TimeDelta::FromSeconds(
      7 * (base::Time::kSecondsPerHour * base::Time::kHoursPerDay));

  const int count = std::count_if(
      ad_events.cbegin(), ad_events.cend(),
      [&now, &time_constraint, &creative_ad](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == ConfirmationType::kServed &&
               ad_event.creative_set_id == creative_ad.creative_set_id &&
               now - ad_event.created_at < time_constraint &&
               DoesAdTypeSupportFrequencyCapping(ad_event.type);
      });

  if (count >= creative_ad.per_week) {
    return false;
  }

  return true;
}

}  // namespace ads
