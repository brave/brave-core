/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/sorts/ad_conversions/ad_conversions_ascending_sort.h"

#include <algorithm>

namespace ads {

AdConversionsAscendingSort::AdConversionsAscendingSort() = default;

AdConversionsAscendingSort::~AdConversionsAscendingSort() = default;

AdConversionList AdConversionsAscendingSort::Apply(
    const AdConversionList& list) const {
  auto sorted_list = list;

  std::sort(sorted_list.begin(), sorted_list.end(),
      [](const AdConversionInfo& a, const AdConversionInfo& b) {
    return a.type == "postview" && b.type == "postclick";
  });

  return sorted_list;
}

}  // namespace ads
