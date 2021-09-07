/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_FILTERS_ADS_HISTORY_FILTER_FACTORY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_FILTERS_ADS_HISTORY_FILTER_FACTORY_H_

#include <memory>

#include "bat/ads/ads_history_info.h"

namespace ads {

class AdsHistoryFilter;

class AdsHistoryFilterFactory {
 public:
  static std::unique_ptr<AdsHistoryFilter> Build(
      const AdsHistoryInfo::FilterType type);
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_FILTERS_ADS_HISTORY_FILTER_FACTORY_H_
