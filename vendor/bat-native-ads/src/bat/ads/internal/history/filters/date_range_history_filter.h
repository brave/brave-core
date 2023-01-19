/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_FILTERS_DATE_RANGE_HISTORY_FILTER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_FILTERS_DATE_RANGE_HISTORY_FILTER_H_

#include "base/time/time.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/history/filters/history_filter_interface.h"

namespace ads {

class DateRangeHistoryFilter final : public HistoryFilterInterface {
 public:
  DateRangeHistoryFilter(base::Time from_time, base::Time to_time);

  HistoryItemList Apply(const HistoryItemList& history) const override;

 private:
  base::Time from_time_;
  base::Time to_time_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_FILTERS_DATE_RANGE_HISTORY_FILTER_H_
