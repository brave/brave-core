/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/sorts/ads_history/ads_history_sort_factory.h"

#include "bat/ads/internal/sorts/ads_history/ads_history_ascending_sort.h"
#include "bat/ads/internal/sorts/ads_history/ads_history_descending_sort.h"

namespace ads {

std::unique_ptr<AdsHistorySort> AdsHistorySortFactory::Build(
    const AdsHistory::SortType type) {
  switch (type) {
    case AdsHistory::SortType::kNone: {
      return nullptr;
    }

    case AdsHistory::SortType::kAscendingOrder: {
      return std::make_unique<AdsHistoryAscendingSort>();
    }

    case AdsHistory::SortType::kDescendingOrder: {
      return std::make_unique<AdsHistoryDescendingSort>();
    }
  }
}

}  // namespace ads
