/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/inline_content_ads_per_hour_frequency_cap.h"

#include "base/time/time.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

namespace ads {

InlineContentAdsPerHourFrequencyCap::InlineContentAdsPerHourFrequencyCap() =
    default;

InlineContentAdsPerHourFrequencyCap::~InlineContentAdsPerHourFrequencyCap() =
    default;

bool InlineContentAdsPerHourFrequencyCap::ShouldAllow() {
  const std::deque<base::Time> history =
      GetAdEvents(AdType::kInlineContentAd, ConfirmationType::kServed);

  if (!DoesRespectCap(history)) {
    last_message_ = "You have exceeded the allowed inline content ads per hour";
    return false;
  }

  return true;
}

std::string InlineContentAdsPerHourFrequencyCap::GetLastMessage() const {
  return last_message_;
}

bool InlineContentAdsPerHourFrequencyCap::DoesRespectCap(
    const std::deque<base::Time>& history) {
  const base::TimeDelta time_constraint =
      base::TimeDelta::FromSeconds(base::Time::kSecondsPerHour);

  const uint64_t cap = features::GetMaximumInlineContentAdsPerHour();

  return DoesHistoryRespectCapForRollingTimeConstraint(history, time_constraint,
                                                       cap);
}

}  // namespace ads
