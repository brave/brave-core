/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_SORTS_ASCENDING_HISTORY_SORT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_SORTS_ASCENDING_HISTORY_SORT_H_

#include "brave/components/brave_ads/core/internal/history/sorts/history_sort_interface.h"

namespace brave_ads {

class AscendingHistorySort final : public HistorySortInterface {
 public:
  void Apply(HistoryItemList& history) const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_SORTS_ASCENDING_HISTORY_SORT_H_
