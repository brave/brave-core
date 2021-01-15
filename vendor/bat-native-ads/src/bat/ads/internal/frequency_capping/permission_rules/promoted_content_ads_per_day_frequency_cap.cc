/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/promoted_content_ads_per_day_frequency_cap.h"

#include <deque>

#include "base/time/time.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

namespace ads {

PromotedContentAdsPerDayFrequencyCap::PromotedContentAdsPerDayFrequencyCap(
    const AdEventList& ad_events)
    : ad_events_(ad_events) {
}

PromotedContentAdsPerDayFrequencyCap::
~PromotedContentAdsPerDayFrequencyCap() = default;

bool PromotedContentAdsPerDayFrequencyCap::ShouldAllow() {
  const AdEventList filtered_ad_events = FilterAdEvents(ad_events_);
  if (!DoesRespectCap(filtered_ad_events)) {
    last_message_ = "You have exceeded the allowed new tab page ads per day";
    return false;
  }

  return true;
}

std::string PromotedContentAdsPerDayFrequencyCap::get_last_message() const {
  return last_message_;
}

bool PromotedContentAdsPerDayFrequencyCap::DoesRespectCap(
    const AdEventList& ad_events) {
  const std::deque<uint64_t> history =
      GetTimestampHistoryForAdEvents(ad_events);

  const uint64_t time_constraint = base::Time::kSecondsPerHour *
      base::Time::kHoursPerDay;

  return DoesHistoryRespectCapForRollingTimeConstraint(
      history, time_constraint, kPromotedContentAdsPerDayFrequencyCap);
}

AdEventList PromotedContentAdsPerDayFrequencyCap::FilterAdEvents(
    const AdEventList& ad_events) const {
  AdEventList filtered_ad_events = ad_events;

  const auto iter = std::remove_if(filtered_ad_events.begin(),
      filtered_ad_events.end(), [](const AdEventInfo& ad_event) {
    return ad_event.type != AdType::kPromotedContentAd ||
        ad_event.confirmation_type != ConfirmationType::kViewed;
  });

  filtered_ad_events.erase(iter, filtered_ad_events.end());

  return filtered_ad_events;
}

}  // namespace ads
