/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_history/sorts/ads_history_descending_sort.h"

#include <algorithm>

namespace ads {

AdsHistoryDescendingSort::AdsHistoryDescendingSort() = default;

AdsHistoryDescendingSort::~AdsHistoryDescendingSort() = default;

std::deque<AdHistoryInfo> AdsHistoryDescendingSort::Apply(
    const std::deque<AdHistoryInfo>& history) const {
  auto sorted_history = history;

  std::sort(sorted_history.begin(), sorted_history.end(),
            [](const AdHistoryInfo& a, const AdHistoryInfo& b) {
              return a.timestamp_in_seconds > b.timestamp_in_seconds;
            });

  return sorted_history;
}

}  // namespace ads
