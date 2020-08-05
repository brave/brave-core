/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/minimum_wait_time_frequency_cap.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

MinimumWaitTimeFrequencyCap::MinimumWaitTimeFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

MinimumWaitTimeFrequencyCap::~MinimumWaitTimeFrequencyCap() = default;

bool MinimumWaitTimeFrequencyCap::IsAllowed() {
  if (PlatformHelper::GetInstance()->IsMobile()) {
    return true;
  }

  const std::deque<AdHistory> history = ads_->get_client()->GetAdsHistory();
  const std::deque<uint64_t> filtered_history = FilterHistory(history);

  if (!DoesRespectCap(filtered_history)) {
    last_message_ = "Ad cannot be shown as the minimum wait time has not "
        "passed";

    return false;
  }

  return true;
}

std::string MinimumWaitTimeFrequencyCap::get_last_message() const {
  return last_message_;
}

bool MinimumWaitTimeFrequencyCap::DoesRespectCap(
    const std::deque<uint64_t>& history) const {
  const uint64_t hour_window = base::Time::kSecondsPerHour;
  const uint64_t hour_allowed = ads_->get_ads_client()->GetAdsPerHour();

  const uint64_t minimum_wait_time = hour_window / hour_allowed;

  return DoesHistoryRespectCapForRollingTimeConstraint(history,
      minimum_wait_time, 1);
}

std::deque<uint64_t> MinimumWaitTimeFrequencyCap::FilterHistory(
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
