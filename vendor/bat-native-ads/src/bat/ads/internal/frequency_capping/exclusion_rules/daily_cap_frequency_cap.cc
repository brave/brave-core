/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/daily_cap_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/frequency_capping.h"
#include "bat/ads/internal/time.h"
#include "bat/ads/internal/client.h"

#include "bat/ads/creative_ad_info.h"

namespace ads {

DailyCapFrequencyCap::DailyCapFrequencyCap(
    const FrequencyCapping* const frequency_capping)
    : frequency_capping_(frequency_capping) {
}

DailyCapFrequencyCap::~DailyCapFrequencyCap() = default;

bool DailyCapFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  if (!DoesAdRespectDailyCampaignCap(ad)) {
    std::ostringstream string_stream;
    string_stream << "campaignId " << ad.campaign_id <<
        " has exceeded the frequency capping for dailyCap";
    last_message_ = string_stream.str();
    return true;
  }
  return false;
}

const std::string DailyCapFrequencyCap::GetLastMessage() const {
    return last_message_;
}

bool DailyCapFrequencyCap::DoesAdRespectDailyCampaignCap(
    const CreativeAdInfo& ad) const {
  auto campaign = frequency_capping_->GetCampaignForUuid(ad.campaign_id);
  auto day_window = base::Time::kSecondsPerHour * base::Time::kHoursPerDay;

  return frequency_capping_->DoesHistoryRespectCapForRollingTimeConstraint(
      campaign, day_window, ad.daily_cap);
}

}  // namespace ads
