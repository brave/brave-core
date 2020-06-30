/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/daily_cap_frequency_cap.h"

#include "bat/ads/internal/creative_ad_info.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_utils.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"

namespace ads {

DailyCapFrequencyCap::DailyCapFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

DailyCapFrequencyCap::~DailyCapFrequencyCap() = default;

bool DailyCapFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  const std::map<std::string, std::deque<uint64_t>> history =
      ads_->get_client()->GetCampaignHistory();

  const std::deque<uint64_t> filtered_history =
      FilterHistory(history, ad.campaign_id);

  if (!DoesRespectCap(filtered_history, ad)) {
    last_message_ = base::StringPrintf("campaignId %s has exceeded the "
        "frequency capping for dailyCap", ad.campaign_id.c_str());

    return true;
  }

  return false;
}

std::string DailyCapFrequencyCap::get_last_message() const {
  return last_message_;
}

bool DailyCapFrequencyCap::DoesRespectCap(
      const std::deque<uint64_t>& history,
      const CreativeAdInfo& ad) const {
  const uint64_t day_window =
      base::Time::kSecondsPerHour * base::Time::kHoursPerDay;

  return DoesHistoryRespectCapForRollingTimeConstraint(
      history, day_window, ad.daily_cap);
}

std::deque<uint64_t> DailyCapFrequencyCap::FilterHistory(
    const std::map<std::string, std::deque<uint64_t>>& history,
    const std::string& campaign_id) {
  std::deque<uint64_t> filtered_history;

  if (history.find(campaign_id) != history.end()) {
    filtered_history = history.at(campaign_id);
  }

  return filtered_history;
}

}  // namespace ads
