/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_inline_content_ad_payload_info.h"

namespace ads {

CatalogInlineContentAdPayloadInfo::CatalogInlineContentAdPayloadInfo() =
    default;

CatalogInlineContentAdPayloadInfo::CatalogInlineContentAdPayloadInfo(
    const CatalogInlineContentAdPayloadInfo& info) = default;

CatalogInlineContentAdPayloadInfo::~CatalogInlineContentAdPayloadInfo() =
    default;

bool CatalogInlineContentAdPayloadInfo::operator==(
    const CatalogInlineContentAdPayloadInfo& rhs) const {
  return title == rhs.title && description == rhs.description &&
         target_url == rhs.target_url;
}

bool CatalogInlineContentAdPayloadInfo::operator!=(
    const CatalogInlineContentAdPayloadInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
