/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_segment_info.h"

#include <tuple>

namespace brave_ads {

bool operator==(const CatalogSegmentInfo& lhs, const CatalogSegmentInfo& rhs) {
  const auto tie = [](const CatalogSegmentInfo& segment) {
    return std::tie(segment.code, segment.name);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const CatalogSegmentInfo& lhs, const CatalogSegmentInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
