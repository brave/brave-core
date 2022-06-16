/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_SORTS_DESCENDING_HISTORY_SORT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_SORTS_DESCENDING_HISTORY_SORT_H_

#include "base/containers/circular_deque.h"
#include "bat/ads/internal/history/sorts/history_sort_interface.h"

namespace ads {

struct HistoryItemInfo;

class DescendingHistorySort final : public HistorySortInterface {
 public:
  DescendingHistorySort();
  ~DescendingHistorySort() override;
  DescendingHistorySort(const DescendingHistorySort&) = delete;
  DescendingHistorySort& operator=(const DescendingHistorySort&) = delete;

  base::circular_deque<HistoryItemInfo> Apply(
      const base::circular_deque<HistoryItemInfo>& history) const override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_SORTS_DESCENDING_HISTORY_SORT_H_
