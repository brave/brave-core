/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/sorts/ascending_history_sort.h"

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"

namespace brave_ads {

HistoryItemList AscendingHistorySort::Apply(
    const HistoryItemList& history) const {
  HistoryItemList sorted_history = history;

  base::ranges::sort(sorted_history, base::ranges::less{},
                     &HistoryItemInfo::created_at);

  return sorted_history;
}

}  // namespace brave_ads
