/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_hour_frequency_cap.h"

#include <cstdint>
#include <deque>

#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"
#include "bat/ads/internal/logging.h"

namespace ads {

namespace {
const uint64_t kPerHourFrequencyCap = 1;
}  // namespace

PerHourFrequencyCap::PerHourFrequencyCap(const AdEventList& ad_events)
    : ad_events_(ad_events) {}

PerHourFrequencyCap::~PerHourFrequencyCap() = default;

bool PerHourFrequencyCap::ShouldExclude(const CreativeAdInfo& ad) {
  const AdEventList filtered_ad_events = FilterAdEvents(ad_events_, ad);

  if (!DoesRespectCap(filtered_ad_events)) {
    last_message_ = base::StringPrintf(
        "creativeInstanceId %s has exceeded the "
        "frequency capping for perHour",
        ad.creative_instance_id.c_str());

    return true;
  }

  return false;
}

std::string PerHourFrequencyCap::get_last_message() const {
  return last_message_;
}

bool PerHourFrequencyCap::DoesRespectCap(const AdEventList& ad_events) {
  const std::deque<uint64_t> history =
      GetTimestampHistoryForAdEvents(ad_events);

  const uint64_t time_constraint = base::Time::kSecondsPerHour;

  return DoesHistoryRespectCapForRollingTimeConstraint(history, time_constraint,
                                                       kPerHourFrequencyCap);
}

AdEventList PerHourFrequencyCap::FilterAdEvents(
    const AdEventList& ad_events,
    const CreativeAdInfo& ad) const {
  AdEventList filtered_ad_events = ad_events;

  const auto iter = std::remove_if(
      filtered_ad_events.begin(), filtered_ad_events.end(),
      [&ad](const AdEventInfo& ad_event) {
        return (ad_event.type != AdType::kAdNotification &&
                ad_event.type != AdType::kInlineContentAd) ||
               ad_event.creative_instance_id != ad.creative_instance_id ||
               ad_event.confirmation_type != ConfirmationType::kServed;
      });

  filtered_ad_events.erase(iter, filtered_ad_events.end());

  return filtered_ad_events;
}

}  // namespace ads
