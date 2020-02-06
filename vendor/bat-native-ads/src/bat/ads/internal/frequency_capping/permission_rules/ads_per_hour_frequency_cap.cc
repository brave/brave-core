/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_hour_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/frequency_capping.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/time.h"
#include "bat/ads/internal/client.h"
#include "bat/ads/internal/ads_impl.h"

#include "bat/ads/creative_ad_notification_info.h"

namespace ads {

AdsPerHourFrequencyCap::AdsPerHourFrequencyCap(
    const AdsImpl* const ads,
    const AdsClient* const ads_client,
    const FrequencyCapping* const frequency_capping)
    : ads_(ads),
      ads_client_(ads_client),
      frequency_capping_(frequency_capping) {
}

AdsPerHourFrequencyCap::~AdsPerHourFrequencyCap() = default;

bool AdsPerHourFrequencyCap::IsAllowed() {
  if (ads_->IsMobile()) {
    return true;
  }

  auto history = frequency_capping_->GetAdsShownHistory();

  auto respects_hour_limit = AreAdsPerHourBelowAllowedThreshold(history);
  if (!respects_hour_limit) {
    last_message_ = "You have exceeded the allowed ads per hour";
    return false;
  }
  return true;
}

const std::string AdsPerHourFrequencyCap::GetLastMessage() const {
    return last_message_;
}

bool AdsPerHourFrequencyCap::AreAdsPerHourBelowAllowedThreshold(
    const std::deque<uint64_t>& history) const {
  auto hour_window = base::Time::kSecondsPerHour;
  auto hour_allowed = ads_client_->GetAdsPerHour();

  auto respects_hour_limit =
      frequency_capping_->DoesHistoryRespectCapForRollingTimeConstraint(
          history, hour_window, hour_allowed);

  return respects_hour_limit;
}

}  // namespace ads
