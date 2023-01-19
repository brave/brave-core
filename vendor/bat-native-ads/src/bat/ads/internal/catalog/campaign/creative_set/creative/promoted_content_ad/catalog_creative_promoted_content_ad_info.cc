/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/creative_set/creative/promoted_content_ad/catalog_creative_promoted_content_ad_info.h"

namespace ads {

bool CatalogCreativePromotedContentAdInfo::operator==(
    const CatalogCreativePromotedContentAdInfo& other) const {
  return CatalogCreativeInfo::operator==(other) && payload == other.payload;
}

bool CatalogCreativePromotedContentAdInfo::operator!=(
    const CatalogCreativePromotedContentAdInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
