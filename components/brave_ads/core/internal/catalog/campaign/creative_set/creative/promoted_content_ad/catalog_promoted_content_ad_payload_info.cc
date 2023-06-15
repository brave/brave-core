/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/promoted_content_ad/catalog_promoted_content_ad_payload_info.h"

#include <tuple>

namespace brave_ads {

bool operator==(const CatalogPromotedContentAdPayloadInfo& lhs,
                const CatalogPromotedContentAdPayloadInfo& rhs) {
  const auto tie = [](const CatalogPromotedContentAdPayloadInfo& payload) {
    return std::tie(payload.title, payload.description, payload.target_url);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const CatalogPromotedContentAdPayloadInfo& lhs,
                const CatalogPromotedContentAdPayloadInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
