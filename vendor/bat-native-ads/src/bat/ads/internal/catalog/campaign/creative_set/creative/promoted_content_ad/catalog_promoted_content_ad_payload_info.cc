/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/creative_set/creative/promoted_content_ad/catalog_promoted_content_ad_payload_info.h"

namespace ads {

bool operator==(const CatalogPromotedContentAdPayloadInfo& lhs,
                const CatalogPromotedContentAdPayloadInfo& rhs) {
  return lhs.title == rhs.title && lhs.description == rhs.description &&
         lhs.target_url == rhs.target_url;
}

bool operator!=(const CatalogPromotedContentAdPayloadInfo& lhs,
                const CatalogPromotedContentAdPayloadInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace ads
