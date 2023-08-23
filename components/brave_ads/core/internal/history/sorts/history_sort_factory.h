/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_SORTS_HISTORY_SORT_FACTORY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_SORTS_HISTORY_SORT_FACTORY_H_

#include <memory>

#include "brave/components/brave_ads/core/internal/history/sorts/history_sort_interface.h"
#include "brave/components/brave_ads/core/public/history/history_sort_types.h"

namespace brave_ads {

class HistorySortFactory final {
 public:
  static std::unique_ptr<HistorySortInterface> Build(HistorySortType type);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_SORTS_HISTORY_SORT_FACTORY_H_
