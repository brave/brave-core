/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_hour_frequency_cap.h"

#include "bat/ads/ad_history.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_utils.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"

namespace ads {

PerHourFrequencyCap::PerHourFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

PerHourFrequencyCap::~PerHourFrequencyCap() = default;

bool PerHourFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  const std::deque<AdHistory> history = ads_->get_client()->GetAdsHistory();
  const std::deque<uint64_t> filtered_history =
      FilterHistory(history, ad.creative_instance_id);

  if (!DoesRespectCap(filtered_history, ad)) {
    last_message_ = base::StringPrintf("creativeInstanceId %s has exceeded the "
        "frequency capping for perHour", ad.creative_instance_id.c_str());

    return true;
  }

  return false;
}

std::string PerHourFrequencyCap::get_last_message() const {
  return last_message_;
}

bool PerHourFrequencyCap::DoesRespectCap(
    const std::deque<uint64_t>& history,
    const CreativeAdInfo& ad) const {
  const uint64_t hour_window = base::Time::kSecondsPerHour;

  return DoesHistoryRespectCapForRollingTimeConstraint(history, hour_window, 1);
}

std::deque<uint64_t> PerHourFrequencyCap::FilterHistory(
    const std::deque<AdHistory>& history,
    const std::string& creative_instance_id) const {
  std::deque<uint64_t> filtered_history;

  for (const auto& ad : history) {
    if (ad.ad_content.creative_instance_id != creative_instance_id ||
        ad.ad_content.ad_action != ConfirmationType::kViewed) {
      continue;
    }

    filtered_history.push_back(ad.timestamp_in_seconds);
  }

  return filtered_history;
}

}  // namespace ads
