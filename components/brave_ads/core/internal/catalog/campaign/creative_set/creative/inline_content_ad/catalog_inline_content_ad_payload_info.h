/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_INLINE_CONTENT_AD_CATALOG_INLINE_CONTENT_AD_PAYLOAD_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_INLINE_CONTENT_AD_CATALOG_INLINE_CONTENT_AD_PAYLOAD_INFO_H_

#include <string>

#include "url/gurl.h"

namespace brave_ads {

struct CatalogInlineContentAdPayloadInfo final {
  CatalogInlineContentAdPayloadInfo();

  CatalogInlineContentAdPayloadInfo(const CatalogInlineContentAdPayloadInfo&);
  CatalogInlineContentAdPayloadInfo& operator=(
      const CatalogInlineContentAdPayloadInfo&);

  CatalogInlineContentAdPayloadInfo(
      CatalogInlineContentAdPayloadInfo&&) noexcept;
  CatalogInlineContentAdPayloadInfo& operator=(
      CatalogInlineContentAdPayloadInfo&&) noexcept;

  ~CatalogInlineContentAdPayloadInfo();

  bool operator==(const CatalogInlineContentAdPayloadInfo&) const = default;

  std::string title;
  std::string description;
  GURL image_url;
  std::string dimensions;
  std::string cta_text;
  GURL target_url;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_INLINE_CONTENT_AD_CATALOG_INLINE_CONTENT_AD_PAYLOAD_INFO_H_
