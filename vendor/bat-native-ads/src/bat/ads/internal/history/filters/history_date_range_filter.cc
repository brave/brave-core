/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/filters/history_date_range_filter.h"

#include "bat/ads/history_item_info.h"

namespace ads {

HistoryDateRangeFilter::HistoryDateRangeFilter(const base::Time from,
                                               const base::Time to)
    : from_(from), to_(to) {}

HistoryDateRangeFilter::~HistoryDateRangeFilter() = default;

base::circular_deque<HistoryItemInfo> HistoryDateRangeFilter::Apply(
    const base::circular_deque<HistoryItemInfo>& history) const {
  base::circular_deque<HistoryItemInfo> filtered_history = history;

  const auto iter = std::remove_if(
      filtered_history.begin(), filtered_history.end(),
      [=](HistoryItemInfo& history_item) {
        return history_item.time < from_ || history_item.time > to_;
      });

  filtered_history.erase(iter, filtered_history.end());

  return filtered_history;
}

}  // namespace ads
