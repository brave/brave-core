/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_SORTS_ADS_HISTORY_ASCENDING_SORT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_SORTS_ADS_HISTORY_ASCENDING_SORT_H_

#include "base/containers/circular_deque.h"
#include "bat/ads/internal/ads_history/sorts/ads_history_sort_interface.h"

namespace ads {

struct AdHistoryInfo;

class AdsHistoryAscendingSort final : public AdsHistorySortInterface {
 public:
  AdsHistoryAscendingSort();
  ~AdsHistoryAscendingSort() override;

  base::circular_deque<AdHistoryInfo> Apply(
      const base::circular_deque<AdHistoryInfo>& history) const override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_SORTS_ADS_HISTORY_ASCENDING_SORT_H_
