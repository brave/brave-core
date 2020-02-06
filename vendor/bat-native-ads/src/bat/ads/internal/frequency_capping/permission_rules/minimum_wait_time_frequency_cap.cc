/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/minimum_wait_time_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/frequency_capping.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/time.h"
#include "bat/ads/internal/client.h"
#include "bat/ads/internal/ads_impl.h"

#include "bat/ads/creative_ad_notification_info.h"

namespace ads {

MinimumWaitTimeFrequencyCap::MinimumWaitTimeFrequencyCap(
    const AdsImpl* const ads,
    const AdsClient* const ads_client,
    const FrequencyCapping* const frequency_capping)
    : ads_(ads),
      ads_client_(ads_client),
      frequency_capping_(frequency_capping) {
}

MinimumWaitTimeFrequencyCap::~MinimumWaitTimeFrequencyCap() = default;

bool MinimumWaitTimeFrequencyCap::IsAllowed() {
  if (ads_->IsMobile()) {
    return true;
  }

  auto history = frequency_capping_->GetAdsShownHistory();

  auto respects_minimum_wait_time = AreAdsAllowedAfterMinimumWaitTime(history);
  if (!respects_minimum_wait_time) {
    last_message_ =
        "Ad cannot be shown as the minimum wait time has not passed";
    return false;
  }

  return true;
}

const std::string MinimumWaitTimeFrequencyCap::GetLastMessage() const {
    return last_message_;
}

bool MinimumWaitTimeFrequencyCap::AreAdsAllowedAfterMinimumWaitTime(
    const std::deque<uint64_t>& history) const {
  auto hour_window = base::Time::kSecondsPerHour;
  auto hour_allowed = ads_client_->GetAdsPerHour();
  auto minimum_wait_time = hour_window / hour_allowed;

  auto respects_minimum_wait_time =
      frequency_capping_->DoesHistoryRespectCapForRollingTimeConstraint(
          history, minimum_wait_time, 1);

  return respects_minimum_wait_time;
}

}  // namespace ads
