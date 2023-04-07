/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_HISTORY_ITEM_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_HISTORY_ITEM_INFO_H_

#include "base/containers/circular_deque.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/ad_content_info.h"
#include "brave/components/brave_ads/core/category_content_info.h"
#include "brave/components/brave_ads/core/export.h"

namespace brave_ads {

struct ADS_EXPORT HistoryItemInfo final {
  base::Time created_at;
  AdContentInfo ad_content;
  CategoryContentInfo category_content;
};

bool operator==(const HistoryItemInfo& lhs, const HistoryItemInfo& rhs);
bool operator!=(const HistoryItemInfo& lhs, const HistoryItemInfo& rhs);

using HistoryItemList = base::circular_deque<HistoryItemInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_HISTORY_ITEM_INFO_H_
