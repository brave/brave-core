/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_HISTORY_CONFIRMATION_FILTER_H_
#define BAT_ADS_INTERNAL_AD_HISTORY_CONFIRMATION_FILTER_H_

#include <deque>
#include <map>
#include <vector>

#include "bat/ads/internal/filters/ad_history_filter.h"

namespace ads {

struct AdsHistory;

class AdHistoryConfirmationFilter : public AdHistoryFilter {
 public :
  AdHistoryConfirmationFilter() = default;
  ~AdHistoryConfirmationFilter() override;

  std::deque<AdHistoryDetail> ApplyFilter(
      const std::deque<AdHistoryDetail>& ad_history_details) const override;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_HISTORY_CONFIRMATION_FILTER_H_
