/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/promoted_content_ads_per_day_frequency_cap.h"

#include "base/time/time.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

namespace ads {

PromotedContentAdsPerDayFrequencyCap::PromotedContentAdsPerDayFrequencyCap() =
    default;

PromotedContentAdsPerDayFrequencyCap::~PromotedContentAdsPerDayFrequencyCap() =
    default;

bool PromotedContentAdsPerDayFrequencyCap::ShouldAllow() {
  const std::deque<uint64_t> history =
      GetAdEvents(AdType::kPromotedContentAd, ConfirmationType::kServed);

  if (!DoesRespectCap(history)) {
    last_message_ =
        "You have exceeded the allowed promoted content ads per day";
    return false;
  }

  return true;
}

std::string PromotedContentAdsPerDayFrequencyCap::get_last_message() const {
  return last_message_;
}

bool PromotedContentAdsPerDayFrequencyCap::DoesRespectCap(
    const std::deque<uint64_t>& history) {
  const uint64_t time_constraint =
      base::Time::kSecondsPerHour * base::Time::kHoursPerDay;

  const uint64_t cap = features::GetMaximumPromotedContentAdsPerDay();

  return DoesHistoryRespectCapForRollingTimeConstraint(history, time_constraint,
                                                       cap);
}

}  // namespace ads
