/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/eligible_ads_filter_factory.h"

#include "bat/ads/internal/eligible_ads/eligible_ads_priority_filter.h"

namespace ads {

std::unique_ptr<EligibleAdsFilter> EligibleAdsFilterFactory::Build(
    const EligibleAdsFilter::Type type) {
  switch (type) {
    case EligibleAdsFilter::Type::kPriority: {
      return std::make_unique<EligibleAdsPriorityFilter>();
    }
  }
}

}  // namespace ads
