/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_day_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/frequency_capping.h"
#include "bat/ads/internal/time.h"
#include "bat/ads/internal/client.h"

#include "bat/ads/creative_ad_info.h"

namespace ads {

PerDayFrequencyCap::PerDayFrequencyCap(
    const FrequencyCapping* const frequency_capping)
    : frequency_capping_(frequency_capping) {
}

PerDayFrequencyCap::~PerDayFrequencyCap() = default;

bool PerDayFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  if (!DoesAdRespectPerDayCap(ad)) {
    std::ostringstream string_stream;
    string_stream << "creativeSetId " << ad.creative_set_id <<
        " has exceeded the frequency capping for perDay";
    last_message_ = string_stream.str();
    return true;
  }
  return false;
}

const std::string PerDayFrequencyCap::GetLastMessage() const {
    return last_message_;
}

bool PerDayFrequencyCap::DoesAdRespectPerDayCap(
    const CreativeAdInfo& ad) const {
  auto creative_set = frequency_capping_->GetCreativeSetHistoryForUuid(
      ad.creative_set_id);
  auto day_window = base::Time::kSecondsPerHour * base::Time::kHoursPerDay;

  return frequency_capping_->DoesHistoryRespectCapForRollingTimeConstraint(
    creative_set, day_window, ad.per_day);
}

}  // namespace ads
