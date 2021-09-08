/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_SORTS_ADS_HISTORY_SORT_FACTORY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_SORTS_ADS_HISTORY_SORT_FACTORY_H_

#include <memory>

#include "bat/ads/ads_history_info.h"

namespace ads {

class AdsHistorySort;

class AdsHistorySortFactory {
 public:
  static std::unique_ptr<AdsHistorySort> Build(
      const AdsHistoryInfo::SortType type);
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_HISTORY_SORTS_ADS_HISTORY_SORT_FACTORY_H_
