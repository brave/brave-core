/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/sorts/conversions_descending_sort.h"

#include "base/ranges/algorithm.h"

namespace ads {

ConversionList ConversionsDescendingSort::Apply(
    const ConversionList& conversions) const {
  ConversionList sorted_conversions = conversions;

  base::ranges::sort(sorted_conversions,
                     [](const ConversionInfo& lhs, const ConversionInfo& rhs) {
                       return lhs.type == "postclick" && rhs.type == "postview";
                     });

  return sorted_conversions;
}

}  // namespace ads
