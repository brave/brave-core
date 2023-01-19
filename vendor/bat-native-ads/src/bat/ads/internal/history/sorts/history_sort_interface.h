/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_SORTS_HISTORY_SORT_INTERFACE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_SORTS_HISTORY_SORT_INTERFACE_H_

#include "bat/ads/history_item_info.h"

namespace ads {

class HistorySortInterface {
 public:
  virtual ~HistorySortInterface() = default;

  virtual HistoryItemList Apply(const HistoryItemList& history) const = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_SORTS_HISTORY_SORT_INTERFACE_H_
