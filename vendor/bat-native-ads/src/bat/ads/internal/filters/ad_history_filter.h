/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_HISTORY_FILTER_H_
#define BAT_ADS_INTERNAL_AD_HISTORY_FILTER_H_

#include <deque>

namespace ads {

struct AdHistoryDetail;

class AdHistoryFilter {
 public:
  virtual ~AdHistoryFilter() = default;

  virtual std::deque<AdHistoryDetail> ApplyFilter(
      const std::deque<AdHistoryDetail>& ad_history_details) const = 0;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_HISTORY_FILTER_H_
