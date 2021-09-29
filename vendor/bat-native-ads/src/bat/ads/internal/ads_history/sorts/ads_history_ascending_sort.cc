/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_history/sorts/ads_history_ascending_sort.h"

#include <algorithm>

#include "bat/ads/ad_history_info.h"

namespace ads {

AdsHistoryAscendingSort::AdsHistoryAscendingSort() = default;

AdsHistoryAscendingSort::~AdsHistoryAscendingSort() = default;

std::deque<AdHistoryInfo> AdsHistoryAscendingSort::Apply(
    const std::deque<AdHistoryInfo>& history) const {
  auto sorted_history = history;

  std::sort(sorted_history.begin(), sorted_history.end(),
            [](const AdHistoryInfo& lhs, const AdHistoryInfo& rhs) {
              return lhs.timestamp < rhs.timestamp;
            });

  return sorted_history;
}

}  // namespace ads
