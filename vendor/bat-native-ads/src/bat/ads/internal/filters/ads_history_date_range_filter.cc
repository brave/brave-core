/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/filters/ads_history_date_range_filter.h"
#include "base/time/time.h"

namespace ads {

AdsHistoryDateRangeFilter::AdsHistoryDateRangeFilter() = default;

AdsHistoryDateRangeFilter::~AdsHistoryDateRangeFilter() = default;

std::deque<AdHistory> AdsHistoryDateRangeFilter::Apply(
    const std::deque<AdHistory>& history,
    const uint64_t from_timestamp,
    const uint64_t to_timestamp) const {
  std::deque<AdHistory> filtered_ads_history;

  for (const auto& entry : history) {
    if (entry.timestamp_in_seconds < from_timestamp ||
        entry.timestamp_in_seconds > to_timestamp) {
      continue;
    }

    filtered_ads_history.push_back(entry);
  }

  return filtered_ads_history;
}

}  // namespace ads
