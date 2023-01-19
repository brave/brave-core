/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/sorts/conversions_sort_factory.h"

#include <ostream>

#include "base/notreached.h"
#include "bat/ads/internal/conversions/sorts/conversions_ascending_sort.h"
#include "bat/ads/internal/conversions/sorts/conversions_descending_sort.h"

namespace ads {

std::unique_ptr<ConversionsSortInterface> ConversionsSortFactory::Build(
    const ConversionSortType type) {
  switch (type) {
    case ConversionSortType::kNone: {
      return nullptr;
    }

    case ConversionSortType::kAscendingOrder: {
      return std::make_unique<ConversionsAscendingSort>();
    }

    case ConversionSortType::kDescendingOrder: {
      return std::make_unique<ConversionsDescendingSort>();
    }
  }

  NOTREACHED() << "Unexpected value for ConversionSortType: "
               << static_cast<int>(type);
  return nullptr;
}

}  // namespace ads
