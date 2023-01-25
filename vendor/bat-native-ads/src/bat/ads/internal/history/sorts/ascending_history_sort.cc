/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/sorts/ascending_history_sort.h"

#include "base/ranges/algorithm.h"
#include "bat/ads/history_item_info.h"

namespace ads {

HistoryItemList AscendingHistorySort::Apply(
    const HistoryItemList& history) const {
  HistoryItemList sorted_history = history;

  base::ranges::sort(sorted_history, base::ranges::less{},
                     &HistoryItemInfo::created_at);

  return sorted_history;
}

}  // namespace ads
