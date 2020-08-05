/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_hour_frequency_cap.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

AdsPerHourFrequencyCap::AdsPerHourFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdsPerHourFrequencyCap::~AdsPerHourFrequencyCap() = default;

bool AdsPerHourFrequencyCap::IsAllowed() {
  if (PlatformHelper::GetInstance()->IsMobile()) {
    return true;
  }

  const std::deque<AdHistory> history = ads_->get_client()->GetAdsHistory();
  const std::deque<uint64_t> filtered_history = FilterHistory(history);

  if (!DoesRespectCap(filtered_history)) {
    last_message_ = "You have exceeded the allowed ads per hour";

    return false;
  }

  return true;
}

std::string AdsPerHourFrequencyCap::get_last_message() const {
  return last_message_;
}

bool AdsPerHourFrequencyCap::DoesRespectCap(
    const std::deque<uint64_t>& history) const {
  const uint64_t hour_window = base::Time::kSecondsPerHour;

  const uint64_t hour_allowed = ads_->get_ads_client()->GetAdsPerHour();

  return DoesHistoryRespectCapForRollingTimeConstraint(history, hour_window,
      hour_allowed);
}

std::deque<uint64_t> AdsPerHourFrequencyCap::FilterHistory(
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
