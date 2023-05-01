/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/history_item_info.h"

#include <limits>

#include "base/numerics/ranges.h"

namespace brave_ads {

// TODO(https://github.com/brave/brave-browser/issues/27893):
// |base::IsApproximatelyEqual| can be removed for timestamp comparisons once
// timestamps are persisted using |base::ValueToTime| and |base::TimeToValue|.
bool operator==(const HistoryItemInfo& lhs, const HistoryItemInfo& rhs) {
  return base::IsApproximatelyEqual(lhs.created_at.ToDoubleT(),
                                    rhs.created_at.ToDoubleT(),
                                    std::numeric_limits<double>::epsilon()) &&
         lhs.ad_content == rhs.ad_content &&
         lhs.category_content == rhs.category_content;
}

bool operator!=(const HistoryItemInfo& lhs, const HistoryItemInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
