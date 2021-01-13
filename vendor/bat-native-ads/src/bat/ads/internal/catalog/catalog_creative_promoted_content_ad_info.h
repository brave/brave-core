/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CATALOG_CATALOG_CREATIVE_PROMOTED_CONTENT_AD_INFO_H_
#define BAT_ADS_INTERNAL_CATALOG_CATALOG_CREATIVE_PROMOTED_CONTENT_AD_INFO_H_

#include <vector>

#include "bat/ads/internal/catalog/catalog_creative_info.h"
#include "bat/ads/internal/catalog/catalog_promoted_content_ad_payload_info.h"

namespace ads {

struct CatalogCreativePromotedContentAdInfo : CatalogCreativeInfo {
  CatalogCreativePromotedContentAdInfo();
  CatalogCreativePromotedContentAdInfo(
      const CatalogCreativePromotedContentAdInfo& info);
  ~CatalogCreativePromotedContentAdInfo();

  bool operator==(
      const CatalogCreativePromotedContentAdInfo& rhs) const;
  bool operator!=(
      const CatalogCreativePromotedContentAdInfo& rhs) const;

  CatalogPromotedContentAdPayloadInfo payload;
};

using CatalogCreativePromotedContentAdList =
    std::vector<CatalogCreativePromotedContentAdInfo>;

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CATALOG_CATALOG_CREATIVE_PROMOTED_CONTENT_AD_INFO_H_
