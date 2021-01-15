/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_creative_promoted_content_ad_info.h"

namespace ads {

CatalogCreativePromotedContentAdInfo::
CatalogCreativePromotedContentAdInfo() = default;

CatalogCreativePromotedContentAdInfo::CatalogCreativePromotedContentAdInfo(
    const CatalogCreativePromotedContentAdInfo& info) = default;

CatalogCreativePromotedContentAdInfo::
~CatalogCreativePromotedContentAdInfo() = default;

bool CatalogCreativePromotedContentAdInfo::operator==(
    const CatalogCreativePromotedContentAdInfo& rhs) const {
  return payload == rhs.payload;
}

bool CatalogCreativePromotedContentAdInfo::operator!=(
    const CatalogCreativePromotedContentAdInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
