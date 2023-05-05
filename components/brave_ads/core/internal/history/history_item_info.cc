/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/history_item_info.h"

#include "brave/components/brave_ads/core/internal/common/numbers/number_util.h"

namespace brave_ads {

bool operator==(const HistoryItemInfo& lhs, const HistoryItemInfo& rhs) {
  return DoubleEquals(lhs.created_at.ToDoubleT(), rhs.created_at.ToDoubleT()) &&
         lhs.ad_content == rhs.ad_content &&
         lhs.category_content == rhs.category_content;
}

bool operator!=(const HistoryItemInfo& lhs, const HistoryItemInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
