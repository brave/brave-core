/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_FILTERS_AD_HISTORY_DATE_RANGE_FILTER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_FILTERS_AD_HISTORY_DATE_RANGE_FILTER_H_

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/history/filters/ad_history_filter_interface.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

class AdHistoryDateRangeFilter final : public AdHistoryFilterInterface {
 public:
  AdHistoryDateRangeFilter(base::Time from_time, base::Time to_time);

  void Apply(AdHistoryList& ad_history) const override;

 private:
  base::Time from_time_;
  base::Time to_time_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_FILTERS_AD_HISTORY_DATE_RANGE_FILTER_H_
