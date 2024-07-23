/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/filters/ad_history_filter_factory.h"

#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/internal/history/filters/ad_history_confirmation_filter.h"

namespace brave_ads {

std::unique_ptr<AdHistoryFilterInterface> AdHistoryFilterFactory::Build(
    const AdHistoryFilterType type) {
  switch (type) {
    case AdHistoryFilterType::kNone: {
      return nullptr;
    }

    case AdHistoryFilterType::kConfirmationType: {
      return std::make_unique<AdHistoryConfirmationFilter>();
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for AdHistoryFilterType: "
                        << base::to_underlying(type);
}

}  // namespace brave_ads
