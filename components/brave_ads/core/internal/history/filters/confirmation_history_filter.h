/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_FILTERS_CONFIRMATION_HISTORY_FILTER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_FILTERS_CONFIRMATION_HISTORY_FILTER_H_

#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/internal/history/filters/history_filter_interface.h"

namespace ads {

class ConfirmationHistoryFilter final : public HistoryFilterInterface {
 public:
  HistoryItemList Apply(const HistoryItemList& history_items) const override;
};

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_FILTERS_CONFIRMATION_HISTORY_FILTER_H_
