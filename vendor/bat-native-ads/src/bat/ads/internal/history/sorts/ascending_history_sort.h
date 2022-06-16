/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_SORTS_ASCENDING_HISTORY_SORT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_SORTS_ASCENDING_HISTORY_SORT_H_

#include "base/containers/circular_deque.h"
#include "bat/ads/internal/history/sorts/history_sort_interface.h"

namespace ads {

struct HistoryItemInfo;

class AscendingHistorySort final : public HistorySortInterface {
 public:
  AscendingHistorySort();
  ~AscendingHistorySort() override;
  AscendingHistorySort(const AscendingHistorySort&) = delete;
  AscendingHistorySort& operator=(const AscendingHistorySort&) = delete;

  base::circular_deque<HistoryItemInfo> Apply(
      const base::circular_deque<HistoryItemInfo>& history) const override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_SORTS_ASCENDING_HISTORY_SORT_H_
