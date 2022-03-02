/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_creative_search_result_ad_info.h"

namespace ads {

CatalogCreativeSearchResultAdInfo::CatalogCreativeSearchResultAdInfo() =
    default;

CatalogCreativeSearchResultAdInfo::CatalogCreativeSearchResultAdInfo(
    const CatalogCreativeSearchResultAdInfo& info) = default;

CatalogCreativeSearchResultAdInfo::~CatalogCreativeSearchResultAdInfo() =
    default;

bool CatalogCreativeSearchResultAdInfo::operator==(
    const CatalogCreativeSearchResultAdInfo& rhs) const {
  return payload == rhs.payload;
}

bool CatalogCreativeSearchResultAdInfo::operator!=(
    const CatalogCreativeSearchResultAdInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
