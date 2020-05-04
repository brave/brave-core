/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_FILTER_FACTORY_H_
#define BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_FILTER_FACTORY_H_

#include <memory>

#include "bat/ads/internal/eligible_ads/eligible_ads_filter.h"

namespace ads {

class EligibleAdsFilterFactory {
 public:
  static std::unique_ptr<EligibleAdsFilter> Build(
      const EligibleAdsFilter::Type type);
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_FILTER_FACTORY_H_
