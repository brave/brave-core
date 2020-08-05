/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/sorts/ad_conversions/ad_conversions_descending_sort.h"

#include <algorithm>

namespace ads {

AdConversionsDescendingSort::AdConversionsDescendingSort() = default;

AdConversionsDescendingSort::~AdConversionsDescendingSort() = default;

AdConversionList AdConversionsDescendingSort::Apply(
    const AdConversionList& list) const {
  auto sorted_list = list;

  std::sort(sorted_list.begin(), sorted_list.end(),
      [](const AdConversionInfo& a, const AdConversionInfo& b) {
    return a.type == "postclick" && b.type == "postview";
  });

  return sorted_list;
}

}  // namespace ads
