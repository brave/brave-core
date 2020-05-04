/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_day_frequency_cap.h"

#include "bat/ads/ad_history.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/client.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_utils.h"

#include "base/time/time.h"

namespace ads {

namespace {

const uint64_t kSecondsPerDay = base::Time::kSecondsPerHour *
    base::Time::kHoursPerDay;

}  // namespace

AdsPerDayFrequencyCap::AdsPerDayFrequencyCap(
    const AdsClient* const ads_client,
    const Client* const client)
    : ads_client_(ads_client),
      client_(client) {
  DCHECK(ads_client_);
  DCHECK(client_);
}

AdsPerDayFrequencyCap::~AdsPerDayFrequencyCap() = default;

bool AdsPerDayFrequencyCap::IsAllowed() {
  const std::deque<AdHistory> history = client_->GetAdsHistory();
  const std::deque<uint64_t> filtered_history = FilterHistory(history);

  if (!DoesRespectCap(filtered_history)) {
    last_message_ = "You have exceeded the allowed ads per day";

    return false;
  }

  return true;
}

std::string AdsPerDayFrequencyCap::get_last_message() const {
  return last_message_;
}

bool AdsPerDayFrequencyCap::DoesRespectCap(
    const std::deque<uint64_t>& history) const {
  const uint64_t day_window = kSecondsPerDay;

  const uint64_t day_allowed = ads_client_->GetAdsPerDay();

  return DoesHistoryRespectCapForRollingTimeConstraint(history, day_window,
      day_allowed);
}

std::deque<uint64_t> AdsPerDayFrequencyCap::FilterHistory(
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
