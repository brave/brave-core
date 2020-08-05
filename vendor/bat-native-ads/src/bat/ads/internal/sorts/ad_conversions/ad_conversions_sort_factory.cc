/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/sorts/ad_conversions/ad_conversions_sort_factory.h"

#include "bat/ads/internal/sorts/ad_conversions/ad_conversions_ascending_sort.h"
#include "bat/ads/internal/sorts/ad_conversions/ad_conversions_descending_sort.h"

namespace ads {

std::unique_ptr<AdConversionsSort> AdConversionsSortFactory::Build(
    const AdConversionInfo::SortType type) {
  switch (type) {
    case AdConversionInfo::SortType::kNone: {
      return nullptr;
    }

    case AdConversionInfo::SortType::kAscendingOrder: {
      return std::make_unique<AdConversionsAscendingSort>();
    }

    case AdConversionInfo::SortType::kDescendingOrder: {
      return std::make_unique<AdConversionsDescendingSort>();
    }
  }
}

}  // namespace ads
