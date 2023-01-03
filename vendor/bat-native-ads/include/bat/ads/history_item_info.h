/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_HISTORY_ITEM_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_HISTORY_ITEM_INFO_H_

#include "base/containers/circular_deque.h"
#include "base/time/time.h"
#include "bat/ads/ad_content_info.h"
#include "bat/ads/category_content_info.h"
#include "bat/ads/export.h"

namespace ads {

struct ADS_EXPORT HistoryItemInfo final {
  base::Time created_at;
  AdContentInfo ad_content;
  CategoryContentInfo category_content;
};

bool operator==(const HistoryItemInfo& lhs, const HistoryItemInfo& rhs);
bool operator!=(const HistoryItemInfo& lhs, const HistoryItemInfo& rhs);

using HistoryItemList = base::circular_deque<HistoryItemInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_HISTORY_ITEM_INFO_H_
