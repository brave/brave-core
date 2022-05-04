/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/sorts/history_ascending_sort.h"

#include <algorithm>

#include "bat/ads/history_item_info.h"

namespace ads {

HistoryAscendingSort::HistoryAscendingSort() = default;

HistoryAscendingSort::~HistoryAscendingSort() = default;

base::circular_deque<HistoryItemInfo> HistoryAscendingSort::Apply(
    const base::circular_deque<HistoryItemInfo>& history) const {
  auto sorted_history = history;

  std::sort(sorted_history.begin(), sorted_history.end(),
            [](const HistoryItemInfo& lhs, const HistoryItemInfo& rhs) {
              return lhs.time < rhs.time;
            });

  return sorted_history;
}

}  // namespace ads
