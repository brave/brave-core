/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_history/filters/ads_history_date_range_filter.h"

namespace ads {

AdsHistoryDateRangeFilter::AdsHistoryDateRangeFilter() = default;

AdsHistoryDateRangeFilter::~AdsHistoryDateRangeFilter() = default;

std::deque<AdHistoryInfo> AdsHistoryDateRangeFilter::Apply(
    const std::deque<AdHistoryInfo>& history,
    const uint64_t from_timestamp,
    const uint64_t to_timestamp) const {
  std::deque<AdHistoryInfo> filtered_ads_history = history;

  const auto iter = std::remove_if(
      filtered_ads_history.begin(), filtered_ads_history.end(),
      [from_timestamp, to_timestamp](AdHistoryInfo& ad_history) {
        return ad_history.timestamp_in_seconds < from_timestamp ||
               ad_history.timestamp_in_seconds > to_timestamp;
      });

  filtered_ads_history.erase(iter, filtered_ads_history.end());

  return filtered_ads_history;
}

}  // namespace ads
