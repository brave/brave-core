/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/landed_frequency_cap.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"
#include "bat/ads/internal/sorts/ads_history/ads_history_sort_factory.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

LandedFrequencyCap::LandedFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

LandedFrequencyCap::~LandedFrequencyCap() = default;

bool LandedFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  const std::deque<AdHistory> history = ads_->get_client()->GetAdsHistory();
  const std::deque<uint64_t> filtered_history =
      FilterHistory(history, ad.campaign_id);

  if (!DoesRespectCap(filtered_history, ad)) {
    last_message_ = base::StringPrintf("campaignId %s has exceeded the "
        "frequency capping for landed", ad.campaign_id.c_str());
    return true;
  }

  return false;
}

std::string LandedFrequencyCap::get_last_message() const {
  return last_message_;
}

bool LandedFrequencyCap::DoesRespectCap(
    const std::deque<uint64_t>& history,
    const CreativeAdInfo& ad) const {
  const uint64_t time_constraint =
      2 * (base::Time::kSecondsPerHour * base::Time::kHoursPerDay);

  const uint64_t cap = 1;

  return DoesHistoryRespectCapForRollingTimeConstraint(history,
      time_constraint, cap);
}

std::deque<uint64_t> LandedFrequencyCap::FilterHistory(
    const std::deque<AdHistory>& history,
    const std::string& campaign_id) {
  std::deque<uint64_t> filtered_history;

  for (const auto& ad : history) {
    if (ad.ad_content.campaign_id != campaign_id ||
        ad.ad_content.ad_action != ConfirmationType::kLanded) {
      continue;
    }

    filtered_history.push_back(ad.timestamp_in_seconds);
  }

  return filtered_history;
}

}  // namespace ads
