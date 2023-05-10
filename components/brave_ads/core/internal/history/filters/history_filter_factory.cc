/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/filters/history_filter_factory.h"

#include <ostream>

#include "base/notreached.h"
#include "brave/components/brave_ads/core/internal/history/filters/confirmation_history_filter.h"

namespace brave_ads {

std::unique_ptr<HistoryFilterInterface> HistoryFilterFactory::Build(
    const HistoryFilterType type) {
  switch (type) {
    case HistoryFilterType::kNone: {
      return nullptr;
    }

    case HistoryFilterType::kConfirmationType: {
      return std::make_unique<ConfirmationHistoryFilter>();
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for HistoryFilterType: "
                        << static_cast<int>(type);
}

}  // namespace brave_ads
