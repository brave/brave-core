/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/filters/ads_history_filter_factory.h"

#include "bat/ads/internal/filters/ads_history_confirmation_filter.h"
#include "bat/ads/internal/filters/ads_history_conversion_filter.h"

namespace ads {

std::unique_ptr<AdsHistoryFilter> AdsHistoryFilterFactory::Build(
    const AdsHistory::FilterType type) {
  switch (type) {
    case AdsHistory::FilterType::kNone: {
      return nullptr;
    }

    case AdsHistory::FilterType::kConfirmationType: {
      return std::make_unique<AdsHistoryConfirmationFilter>();
    }

    case AdsHistory::FilterType::kAdConversion: {
      return std::make_unique<AdsHistoryConversionFilter>();
    }
  }
}

}  // namespace ads
