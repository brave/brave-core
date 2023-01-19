/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_INLINE_CONTENT_AD_CATALOG_CREATIVE_INLINE_CONTENT_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_INLINE_CONTENT_AD_CATALOG_CREATIVE_INLINE_CONTENT_AD_INFO_H_

#include <vector>

#include "bat/ads/internal/catalog/campaign/creative_set/creative/catalog_creative_info.h"
#include "bat/ads/internal/catalog/campaign/creative_set/creative/inline_content_ad/catalog_inline_content_ad_payload_info.h"

namespace ads {

struct CatalogCreativeInlineContentAdInfo final : CatalogCreativeInfo {
  bool operator==(const CatalogCreativeInlineContentAdInfo& other) const;
  bool operator!=(const CatalogCreativeInlineContentAdInfo& other) const;

  CatalogInlineContentAdPayloadInfo payload;
};

using CatalogCreativeInlineContentAdList =
    std::vector<CatalogCreativeInlineContentAdInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_INLINE_CONTENT_AD_CATALOG_CREATIVE_INLINE_CONTENT_AD_INFO_H_
