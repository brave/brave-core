/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/history_item_info.h"

#include "bat/ads/internal/base/numbers/number_util.h"

namespace ads {

bool operator==(const HistoryItemInfo& lhs, const HistoryItemInfo& rhs) {
  return DoubleEquals(lhs.created_at.ToDoubleT(), rhs.created_at.ToDoubleT()) &&
         lhs.ad_content == rhs.ad_content &&
         lhs.category_content == rhs.category_content;
}

bool operator!=(const HistoryItemInfo& lhs, const HistoryItemInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace ads
