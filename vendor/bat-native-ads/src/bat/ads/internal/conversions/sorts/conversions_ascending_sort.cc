/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/sorts/conversions_ascending_sort.h"

#include <algorithm>

namespace ads {

ConversionsAscendingSort::ConversionsAscendingSort() = default;

ConversionsAscendingSort::~ConversionsAscendingSort() = default;

ConversionList ConversionsAscendingSort::Apply(
    const ConversionList& conversions) const {
  ConversionList sorted_conversions = conversions;

  std::sort(sorted_conversions.begin(), sorted_conversions.end(),
            [](const ConversionInfo& lhs, const ConversionInfo& rhs) {
              return lhs.type == "postview" && rhs.type == "postclick";
            });

  return sorted_conversions;
}

}  // namespace ads
