/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/history_item_info.h"

#include <tuple>

namespace brave_ads {

bool operator==(const HistoryItemInfo& lhs, const HistoryItemInfo& rhs) {
  const auto tie = [](const HistoryItemInfo& history_item) {
    return std::tie(history_item.created_at, history_item.ad_content,
                    history_item.category_content);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const HistoryItemInfo& lhs, const HistoryItemInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
