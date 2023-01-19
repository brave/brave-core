/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/creative_set/creative/inline_content_ad/catalog_inline_content_ad_payload_info.h"

namespace ads {

CatalogInlineContentAdPayloadInfo::CatalogInlineContentAdPayloadInfo() =
    default;

CatalogInlineContentAdPayloadInfo::CatalogInlineContentAdPayloadInfo(
    const CatalogInlineContentAdPayloadInfo& other) = default;

CatalogInlineContentAdPayloadInfo& CatalogInlineContentAdPayloadInfo::operator=(
    const CatalogInlineContentAdPayloadInfo& other) = default;

CatalogInlineContentAdPayloadInfo::CatalogInlineContentAdPayloadInfo(
    CatalogInlineContentAdPayloadInfo&& other) noexcept = default;

CatalogInlineContentAdPayloadInfo& CatalogInlineContentAdPayloadInfo::operator=(
    CatalogInlineContentAdPayloadInfo&& other) noexcept = default;

CatalogInlineContentAdPayloadInfo::~CatalogInlineContentAdPayloadInfo() =
    default;

bool CatalogInlineContentAdPayloadInfo::operator==(
    const CatalogInlineContentAdPayloadInfo& other) const {
  return title == other.title && description == other.description &&
         image_url == other.image_url && dimensions == other.dimensions &&
         cta_text == other.cta_text && target_url == other.target_url;
}

bool CatalogInlineContentAdPayloadInfo::operator!=(
    const CatalogInlineContentAdPayloadInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
