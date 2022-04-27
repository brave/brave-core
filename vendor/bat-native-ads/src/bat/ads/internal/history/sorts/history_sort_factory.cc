/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/sorts/history_sort_factory.h"

#include "bat/ads/internal/history/sorts/history_ascending_sort.h"
#include "bat/ads/internal/history/sorts/history_descending_sort.h"

namespace ads {

std::unique_ptr<HistorySortInterface> HistorySortFactory::Build(
    const HistorySortType type) {
  switch (type) {
    case HistorySortType::kNone: {
      return nullptr;
    }

    case HistorySortType::kAscendingOrder: {
      return std::make_unique<HistoryAscendingSort>();
    }

    case HistorySortType::kDescendingOrder: {
      return std::make_unique<HistoryDescendingSort>();
    }
  }
}

}  // namespace ads
