/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_FILTERS_ADS_HISTORY_CONFIRMATION_FILTER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_FILTERS_ADS_HISTORY_CONFIRMATION_FILTER_H_

#include <deque>

#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads_history/filters/ads_history_filter.h"

namespace ads {

class AdsHistoryConfirmationFilter : public AdsHistoryFilter {
 public:
  AdsHistoryConfirmationFilter();
  ~AdsHistoryConfirmationFilter() override;

  std::deque<AdHistoryInfo> Apply(
      const std::deque<AdHistoryInfo>& history) const override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_FILTERS_ADS_HISTORY_CONFIRMATION_FILTER_H_
