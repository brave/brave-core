/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_search_result_ad_payload_info.h"

namespace ads {

CatalogSearchResultAdPayloadInfo::CatalogSearchResultAdPayloadInfo() = default;

CatalogSearchResultAdPayloadInfo::CatalogSearchResultAdPayloadInfo(
    const CatalogSearchResultAdPayloadInfo& info) = default;

CatalogSearchResultAdPayloadInfo::~CatalogSearchResultAdPayloadInfo() = default;

bool CatalogSearchResultAdPayloadInfo::operator==(
    const CatalogSearchResultAdPayloadInfo& rhs) const {
  return title == rhs.title && body == rhs.body && target_url == rhs.target_url;
}

bool CatalogSearchResultAdPayloadInfo::operator!=(
    const CatalogSearchResultAdPayloadInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
