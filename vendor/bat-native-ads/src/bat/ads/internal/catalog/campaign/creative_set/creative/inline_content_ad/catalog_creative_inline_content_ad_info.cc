/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/creative_set/creative/inline_content_ad/catalog_creative_inline_content_ad_info.h"

namespace ads {

CatalogCreativeInlineContentAdInfo::CatalogCreativeInlineContentAdInfo() =
    default;

CatalogCreativeInlineContentAdInfo::CatalogCreativeInlineContentAdInfo(
    const CatalogCreativeInlineContentAdInfo& info) = default;

CatalogCreativeInlineContentAdInfo&
CatalogCreativeInlineContentAdInfo::operator=(
    const CatalogCreativeInlineContentAdInfo& info) = default;

CatalogCreativeInlineContentAdInfo::CatalogCreativeInlineContentAdInfo(
    CatalogCreativeInlineContentAdInfo&& other) noexcept = default;

CatalogCreativeInlineContentAdInfo&
CatalogCreativeInlineContentAdInfo::operator=(
    CatalogCreativeInlineContentAdInfo&& other) noexcept = default;

CatalogCreativeInlineContentAdInfo::~CatalogCreativeInlineContentAdInfo() =
    default;

bool CatalogCreativeInlineContentAdInfo::operator==(
    const CatalogCreativeInlineContentAdInfo& other) const {
  return CatalogCreativeInfo::operator==(other) && payload == other.payload;
}

bool CatalogCreativeInlineContentAdInfo::operator!=(
    const CatalogCreativeInlineContentAdInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
