/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_day_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/frequency_capping.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/time.h"
#include "bat/ads/internal/client.h"

#include "bat/ads/creative_ad_notification_info.h"

namespace ads {

AdsPerDayFrequencyCap::AdsPerDayFrequencyCap(
    const AdsClient* const ads_client,
    const FrequencyCapping* const frequency_capping)
    : ads_client_(ads_client),
      frequency_capping_(frequency_capping) {
}

AdsPerDayFrequencyCap::~AdsPerDayFrequencyCap() = default;

bool AdsPerDayFrequencyCap::IsAllowed() {
  auto respects_day_limit = AreAdsPerDayBelowAllowedThreshold();

  if (!respects_day_limit) {
    last_message_ = "You have exceeded the allowed ads per day";
  }

  return respects_day_limit;
}

const std::string AdsPerDayFrequencyCap::GetLastMessage() const {
    return last_message_;
}

bool AdsPerDayFrequencyCap::AreAdsPerDayBelowAllowedThreshold() const {
  auto history = frequency_capping_->GetAdsShownHistory();

  auto day_window = base::Time::kSecondsPerHour * base::Time::kHoursPerDay;
  auto day_allowed = ads_client_->GetAdsPerDay();

  auto respects_day_limit =
      frequency_capping_->DoesHistoryRespectCapForRollingTimeConstraint(
          history, day_window, day_allowed);

  return respects_day_limit;
}

}  // namespace ads
