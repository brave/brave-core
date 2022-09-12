/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/creative_set/catalog_segment_info.h"

namespace ads {

bool operator==(const CatalogSegmentInfo& lhs, const CatalogSegmentInfo& rhs) {
  return lhs.code == rhs.code && lhs.name == rhs.name;
}

bool operator!=(const CatalogSegmentInfo& lhs, const CatalogSegmentInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace ads
