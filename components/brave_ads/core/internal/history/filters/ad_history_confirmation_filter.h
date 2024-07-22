/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_FILTERS_AD_HISTORY_CONFIRMATION_FILTER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_FILTERS_AD_HISTORY_CONFIRMATION_FILTER_H_

#include "brave/components/brave_ads/core/internal/history/filters/ad_history_filter_interface.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

class AdHistoryConfirmationFilter final : public AdHistoryFilterInterface {
 public:
  void Apply(AdHistoryList& ad_history) const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_FILTERS_AD_HISTORY_CONFIRMATION_FILTER_H_
