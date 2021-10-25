/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_hour_frequency_cap.h"

#include <algorithm>

#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

namespace ads {

namespace {
const int kPerHourFrequencyCap = 1;
}  // namespace

PerHourFrequencyCap::PerHourFrequencyCap(const AdEventList& ad_events)
    : ad_events_(ad_events) {}

PerHourFrequencyCap::~PerHourFrequencyCap() = default;

std::string PerHourFrequencyCap::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_instance_id;
}

bool PerHourFrequencyCap::ShouldExclude(const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(ad_events_, creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeInstanceId %s has exceeded the perHour frequency cap",
        creative_ad.creative_instance_id.c_str());

    return true;
  }

  return false;
}

std::string PerHourFrequencyCap::GetLastMessage() const {
  return last_message_;
}

bool PerHourFrequencyCap::DoesRespectCap(const AdEventList& ad_events,
                                         const CreativeAdInfo& creative_ad) {
  const base::Time now = base::Time::Now();

  const base::TimeDelta time_constraint =
      base::Seconds(base::Time::kSecondsPerHour);

  const int count = std::count_if(
      ad_events.cbegin(), ad_events.cend(),
      [&now, &time_constraint, &creative_ad](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == ConfirmationType::kServed &&
               ad_event.creative_instance_id ==
                   creative_ad.creative_instance_id &&
               now - ad_event.created_at < time_constraint &&
               DoesAdTypeSupportFrequencyCapping(ad_event.type);
      });

  if (count >= kPerHourFrequencyCap) {
    return false;
  }

  return true;
}

}  // namespace ads
