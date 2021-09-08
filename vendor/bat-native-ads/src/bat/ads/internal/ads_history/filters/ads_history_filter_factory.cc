/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_history/filters/ads_history_filter_factory.h"

#include "bat/ads/internal/ads_history/filters/ads_history_confirmation_filter.h"
#include "bat/ads/internal/ads_history/filters/ads_history_filter.h"

namespace ads {

std::unique_ptr<AdsHistoryFilter> AdsHistoryFilterFactory::Build(
    const AdsHistoryInfo::FilterType type) {
  switch (type) {
    case AdsHistoryInfo::FilterType::kNone: {
      return nullptr;
    }

    case AdsHistoryInfo::FilterType::kConfirmationType: {
      return std::make_unique<AdsHistoryConfirmationFilter>();
    }
  }
}

}  // namespace ads
