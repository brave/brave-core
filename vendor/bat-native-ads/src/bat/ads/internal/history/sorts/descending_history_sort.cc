/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/sorts/descending_history_sort.h"

#include <algorithm>

#include "bat/ads/history_item_info.h"

namespace ads {

DescendingHistorySort::DescendingHistorySort() = default;

DescendingHistorySort::~DescendingHistorySort() = default;

base::circular_deque<HistoryItemInfo> DescendingHistorySort::Apply(
    const base::circular_deque<HistoryItemInfo>& history) const {
  base::circular_deque<HistoryItemInfo> sorted_history = history;

  std::sort(sorted_history.begin(), sorted_history.end(),
            [](const HistoryItemInfo& lhs, const HistoryItemInfo& rhs) {
              return lhs.created_at > rhs.created_at;
            });

  return sorted_history;
}

}  // namespace ads
