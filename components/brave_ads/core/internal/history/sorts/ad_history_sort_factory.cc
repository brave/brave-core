/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/internal/history/sorts/ad_history_ascending_sort.h"
#include "brave/components/brave_ads/core/internal/history/sorts/ad_history_descending_sort.h"
#include "brave/components/brave_ads/core/internal/history/sorts/ad_history_sort_factory.h"

namespace brave_ads {

std::unique_ptr<AdHistorySortInterface> AdHistorySortFactory::Build(
    const AdHistorySortType type) {
  switch (type) {
    case AdHistorySortType::kNone: {
      return nullptr;
    }

    case AdHistorySortType::kAscendingOrder: {
      return std::make_unique<AdHistoryAscendingSort>();
    }

    case AdHistorySortType::kDescendingOrder: {
      return std::make_unique<AdHistoryDescendingSort>();
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for AdHistorySortType: "
                        << base::to_underlying(type);
}

}  // namespace brave_ads
