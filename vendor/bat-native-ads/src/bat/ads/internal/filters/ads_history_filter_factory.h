/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FILTERS_ADS_HISTORY_FILTER_FACTORY_H_
#define BAT_ADS_INTERNAL_FILTERS_ADS_HISTORY_FILTER_FACTORY_H_

#include <memory>

#include "bat/ads/ads_history.h"
#include "bat/ads/internal/filters/ads_history_filter.h"

namespace ads {

class AdsHistoryFilterFactory {
 public:
  static std::unique_ptr<AdsHistoryFilter> Build(
      const AdsHistory::FilterType type);
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FILTERS_ADS_HISTORY_FILTER_FACTORY_H_
