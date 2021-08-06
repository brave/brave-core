/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_SORTS_ADS_HISTORY_DESCENDING_SORT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_SORTS_ADS_HISTORY_DESCENDING_SORT_H_

#include <deque>

#include "bat/ads/internal/ads_history/sorts/ads_history_sort.h"

namespace ads {

class AdsHistoryDescendingSort : public AdsHistorySort {
 public:
  AdsHistoryDescendingSort();
  ~AdsHistoryDescendingSort() override;

  std::deque<AdHistoryInfo> Apply(
      const std::deque<AdHistoryInfo>& history) const override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_SORTS_ADS_HISTORY_DESCENDING_SORT_H_
