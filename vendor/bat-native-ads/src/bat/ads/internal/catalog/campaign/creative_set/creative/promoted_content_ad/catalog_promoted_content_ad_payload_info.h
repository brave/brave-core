/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_PROMOTED_CONTENT_AD_CATALOG_PROMOTED_CONTENT_AD_PAYLOAD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_PROMOTED_CONTENT_AD_CATALOG_PROMOTED_CONTENT_AD_PAYLOAD_INFO_H_

#include <string>

#include "url/gurl.h"

namespace ads {

struct CatalogPromotedContentAdPayloadInfo final {
  std::string title;
  std::string description;
  GURL target_url;
};

bool operator==(const CatalogPromotedContentAdPayloadInfo& lhs,
                const CatalogPromotedContentAdPayloadInfo& rhs);
bool operator!=(const CatalogPromotedContentAdPayloadInfo& lhs,
                const CatalogPromotedContentAdPayloadInfo& rhs);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_PROMOTED_CONTENT_AD_CATALOG_PROMOTED_CONTENT_AD_PAYLOAD_INFO_H_
