/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_creative_inline_content_ad_info.h"

namespace ads {

CatalogCreativeInlineContentAdInfo::CatalogCreativeInlineContentAdInfo() =
    default;

CatalogCreativeInlineContentAdInfo::CatalogCreativeInlineContentAdInfo(
    const CatalogCreativeInlineContentAdInfo& info) = default;

CatalogCreativeInlineContentAdInfo::~CatalogCreativeInlineContentAdInfo() =
    default;

bool CatalogCreativeInlineContentAdInfo::operator==(
    const CatalogCreativeInlineContentAdInfo& rhs) const {
  return payload == rhs.payload;
}

bool CatalogCreativeInlineContentAdInfo::operator!=(
    const CatalogCreativeInlineContentAdInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
