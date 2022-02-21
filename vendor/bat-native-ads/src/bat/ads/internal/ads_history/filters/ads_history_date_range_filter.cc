/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_history/filters/ads_history_date_range_filter.h"

#include "base/time/time.h"
#include "bat/ads/ad_history_info.h"

namespace ads {

AdsHistoryDateRangeFilter::AdsHistoryDateRangeFilter() = default;

AdsHistoryDateRangeFilter::~AdsHistoryDateRangeFilter() = default;

std::deque<AdHistoryInfo> AdsHistoryDateRangeFilter::Apply(
    const std::deque<AdHistoryInfo>& history,
    const base::Time& from,
    const base::Time& to) const {
  std::deque<AdHistoryInfo> filtered_ads_history = history;

  const auto iter = std::remove_if(
      filtered_ads_history.begin(), filtered_ads_history.end(),
      [&from, &to](AdHistoryInfo& ad_history) {
        const base::Time time = base::Time::FromDoubleT(ad_history.timestamp);
        return time < from || time > to;
      });

  filtered_ads_history.erase(iter, filtered_ads_history.end());

  return filtered_ads_history;
}

}  // namespace ads
