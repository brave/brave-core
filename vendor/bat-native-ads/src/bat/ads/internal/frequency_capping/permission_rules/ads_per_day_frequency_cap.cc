/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_day_frequency_cap.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

AdsPerDayFrequencyCap::AdsPerDayFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdsPerDayFrequencyCap::~AdsPerDayFrequencyCap() = default;

bool AdsPerDayFrequencyCap::IsAllowed() {
  const std::deque<AdHistory> history = ads_->get_client()->GetAdsHistory();
  const std::deque<uint64_t> filtered_history = FilterHistory(history);

  if (!DoesRespectCap(filtered_history)) {
    last_message_ = "You have exceeded the allowed ads per day";

    return false;
  }

  return true;
}

std::string AdsPerDayFrequencyCap::get_last_message() const {
  return last_message_;
}

bool AdsPerDayFrequencyCap::DoesRespectCap(
    const std::deque<uint64_t>& history) {
  const uint64_t time_constraint = base::Time::kSecondsPerHour *
      base::Time::kHoursPerDay;

  const uint64_t cap = ads_->get_ads_client()->GetAdsPerDay();

  return DoesHistoryRespectCapForRollingTimeConstraint(history,
      time_constraint, cap);
}

std::deque<uint64_t> AdsPerDayFrequencyCap::FilterHistory(
    const std::deque<AdHistory>& history) const {
  std::deque<uint64_t> filtered_history;

  for (const auto& ad : history) {
    if (ad.ad_content.ad_action != ConfirmationType::kViewed) {
      continue;
    }

    filtered_history.push_back(ad.timestamp_in_seconds);
  }

  return filtered_history;
}

}  // namespace ads
