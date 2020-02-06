/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_hour_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/frequency_capping.h"
#include "bat/ads/internal/time.h"
#include "bat/ads/internal/client.h"

#include "bat/ads/creative_ad_info.h"

namespace ads {

PerHourFrequencyCap::PerHourFrequencyCap(
    const FrequencyCapping* const frequency_capping)
    : frequency_capping_(frequency_capping) {
}

PerHourFrequencyCap::~PerHourFrequencyCap() = default;

bool PerHourFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  if (!DoesAdRespectPerHourCap(ad)) {
    std::ostringstream string_stream;
    string_stream << "adUUID " << ad.creative_instance_id <<
        " has exceeded the frequency capping for perHour";
    last_message_ = string_stream.str();
    return true;
  }
  return false;
}

const std::string PerHourFrequencyCap::GetLastMessage() const {
    return last_message_;
}

bool PerHourFrequencyCap::DoesAdRespectPerHourCap(
    const CreativeAdInfo& ad) const {
  auto ads_shown =
      frequency_capping_->GetAdsHistoryForUuid(ad.creative_instance_id);
  auto hour_window = base::Time::kSecondsPerHour;

  return frequency_capping_->DoesHistoryRespectCapForRollingTimeConstraint(
      ads_shown, hour_window, 1);
}

}  // namespace ads
