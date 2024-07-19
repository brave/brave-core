/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_FILTERS_AD_HISTORY_FILTER_FACTORY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_FILTERS_AD_HISTORY_FILTER_FACTORY_H_

#include <memory>

#include "brave/components/brave_ads/core/internal/history/filters/ad_history_filter_interface.h"
#include "brave/components/brave_ads/core/public/history/ad_history_filter_types.h"

namespace brave_ads {

class AdHistoryFilterFactory final {
 public:
  static std::unique_ptr<AdHistoryFilterInterface> Build(
      AdHistoryFilterType type);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_FILTERS_AD_HISTORY_FILTER_FACTORY_H_
