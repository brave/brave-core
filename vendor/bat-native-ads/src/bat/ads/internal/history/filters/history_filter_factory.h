/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_FILTERS_HISTORY_FILTER_FACTORY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_FILTERS_HISTORY_FILTER_FACTORY_H_

#include <memory>

#include "bat/ads/history_filter_types.h"
#include "bat/ads/internal/history/filters/history_filter_interface.h"

namespace ads {

class HistoryFilterFactory final {
 public:
  static std::unique_ptr<HistoryFilterInterface> Build(HistoryFilterType type);
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_FILTERS_HISTORY_FILTER_FACTORY_H_
