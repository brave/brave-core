/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/dismissed_frequency_cap.h"

#include <algorithm>
#include <iterator>

#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_features.h"

namespace ads {

DismissedFrequencyCap::DismissedFrequencyCap(const AdEventList& ad_events)
    : ad_events_(ad_events) {}

DismissedFrequencyCap::~DismissedFrequencyCap() = default;

std::string DismissedFrequencyCap::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.campaign_id;
}

bool DismissedFrequencyCap::ShouldExclude(const CreativeAdInfo& creative_ad) {
  const AdEventList filtered_ad_events =
      FilterAdEvents(ad_events_, creative_ad);

  if (!DoesRespectCap(filtered_ad_events)) {
    last_message_ = base::StringPrintf(
        "campaignId %s has exceeded the dismissed frequency cap",
        creative_ad.campaign_id.c_str());

    return true;
  }

  return false;
}

std::string DismissedFrequencyCap::GetLastMessage() const {
  return last_message_;
}

bool DismissedFrequencyCap::DoesRespectCap(const AdEventList& ad_events) {
  int count = 0;

  for (const auto& ad_event : ad_events) {
    if (ad_event.confirmation_type == ConfirmationType::kClicked) {
      count = 0;
    } else if (ad_event.confirmation_type == ConfirmationType::kDismissed) {
      count++;
      if (count >= 2) {
        // An ad was dismissed two or more times in a row without being clicked,
        // so do not show another ad from the same campaign for the specified
        // hours
        return false;
      }
    }
  }

  return true;
}

AdEventList DismissedFrequencyCap::FilterAdEvents(
    const AdEventList& ad_events,
    const CreativeAdInfo& creative_ad) const {
  const base::Time now = base::Time::Now();

  const base::TimeDelta time_constraint =
      features::frequency_capping::ExcludeAdIfDismissedWithinTimeWindow();

  AdEventList filtered_ad_events;
  std::copy_if(
      ad_events.cbegin(), ad_events.cend(),
      std::back_inserter(filtered_ad_events),
      [&now, &time_constraint, &creative_ad](const AdEventInfo& ad_event) {
        return (ad_event.confirmation_type == ConfirmationType::kClicked ||
                ad_event.confirmation_type == ConfirmationType::kDismissed) &&
               ad_event.type == AdType::kAdNotification &&
               ad_event.campaign_id == creative_ad.campaign_id &&
               now - ad_event.created_at < time_constraint;
      });

  return filtered_ad_events;
}

}  // namespace ads
