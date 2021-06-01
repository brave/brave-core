/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_FILTERS_ELIGIBLE_ADS_FILTER_FACTORY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_FILTERS_ELIGIBLE_ADS_FILTER_FACTORY_H_

#include <memory>

#include "bat/ads/internal/eligible_ads/inline_content_ads/filters/eligible_ads_filter.h"

namespace ads {

class EligibleAdsFilterFactory {
 public:
  static std::unique_ptr<EligibleAdsFilter> Build(
      const EligibleAdsFilter::Type type);
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_FILTERS_ELIGIBLE_ADS_FILTER_FACTORY_H_
