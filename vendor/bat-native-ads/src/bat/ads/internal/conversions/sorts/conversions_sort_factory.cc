/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/sorts/conversions_sort_factory.h"

#include "bat/ads/internal/conversions/sorts/conversions_ascending_sort.h"
#include "bat/ads/internal/conversions/sorts/conversions_descending_sort.h"

namespace ads {

std::unique_ptr<ConversionsSort> ConversionsSortFactory::Build(
    const ConversionInfo::SortType type) {
  switch (type) {
    case ConversionInfo::SortType::kNone: {
      return nullptr;
    }

    case ConversionInfo::SortType::kAscendingOrder: {
      return std::make_unique<ConversionsAscendingSort>();
    }

    case ConversionInfo::SortType::kDescendingOrder: {
      return std::make_unique<ConversionsDescendingSort>();
    }
  }
}

}  // namespace ads
