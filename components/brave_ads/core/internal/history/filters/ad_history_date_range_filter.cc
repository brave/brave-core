/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/filters/ad_history_date_range_filter.h"

namespace brave_ads {

AdHistoryDateRangeFilter::AdHistoryDateRangeFilter(const base::Time from_time,
                                                   const base::Time to_time)
    : from_time_(from_time), to_time_(to_time) {}

void AdHistoryDateRangeFilter::Apply(AdHistoryList& ad_history) const {
  base::EraseIf(ad_history, [&](const AdHistoryItemInfo& ad_history_item) {
    return ad_history_item.created_at < from_time_ ||
           ad_history_item.created_at > to_time_;
  });
}

}  // namespace brave_ads
