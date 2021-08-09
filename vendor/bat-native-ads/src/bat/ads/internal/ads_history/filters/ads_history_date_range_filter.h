/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_FILTERS_ADS_HISTORY_DATE_RANGE_FILTER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_FILTERS_ADS_HISTORY_DATE_RANGE_FILTER_H_

#include <cstdint>
#include <deque>

#include "bat/ads/ad_history_info.h"

namespace ads {

class AdsHistoryDateRangeFilter {
 public:
  AdsHistoryDateRangeFilter();
  ~AdsHistoryDateRangeFilter();

  std::deque<AdHistoryInfo> Apply(const std::deque<AdHistoryInfo>& history,
                                  const uint64_t from_timestamp,
                                  const uint64_t to_timestamp) const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_FILTERS_ADS_HISTORY_DATE_RANGE_FILTER_H_
