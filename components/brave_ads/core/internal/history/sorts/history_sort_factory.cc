/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/sorts/history_sort_factory.h"

#include <ostream>

#include "base/notreached.h"
#include "brave/components/brave_ads/core/internal/history/sorts/ascending_history_sort.h"
#include "brave/components/brave_ads/core/internal/history/sorts/descending_history_sort.h"

namespace brave_ads {

std::unique_ptr<HistorySortInterface> HistorySortFactory::Build(
    const HistorySortType type) {
  switch (type) {
    case HistorySortType::kNone: {
      return nullptr;
    }

    case HistorySortType::kAscendingOrder: {
      return std::make_unique<AscendingHistorySort>();
    }

    case HistorySortType::kDescendingOrder: {
      return std::make_unique<DescendingHistorySort>();
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for HistorySortType: "
                        << static_cast<int>(type);
}

}  // namespace brave_ads
