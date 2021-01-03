/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_creative_set_info.h"

namespace ads {

CatalogCreativeSetInfo::CatalogCreativeSetInfo() = default;

CatalogCreativeSetInfo::CatalogCreativeSetInfo(
    const CatalogCreativeSetInfo& info) = default;

CatalogCreativeSetInfo::~CatalogCreativeSetInfo() = default;

bool CatalogCreativeSetInfo::operator==(
    const CatalogCreativeSetInfo& rhs) const {
  return creative_set_id == rhs.creative_set_id &&
      per_day == rhs.per_day &&
      total_max == rhs.total_max &&
      segments == rhs.segments &&
      oses == rhs.oses &&
      creative_ad_notifications == rhs.creative_ad_notifications &&
      creative_new_tab_page_ads == rhs.creative_new_tab_page_ads &&
      conversions == rhs.conversions;
}

bool CatalogCreativeSetInfo::operator!=(
    const CatalogCreativeSetInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
