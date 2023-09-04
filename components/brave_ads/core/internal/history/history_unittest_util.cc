/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/history_unittest_util.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/public/history/history_filter_types.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"
#include "brave/components/brave_ads/core/public/history/history_sort_types.h"

namespace brave_ads {

size_t GetHistoryItemCountForTesting() {
  const HistoryItemList history_items =
      HistoryManager::Get(HistoryFilterType::kNone, HistorySortType::kNone,
                          DistantPast(), DistantFuture());

  return history_items.size();
}

}  // namespace brave_ads
