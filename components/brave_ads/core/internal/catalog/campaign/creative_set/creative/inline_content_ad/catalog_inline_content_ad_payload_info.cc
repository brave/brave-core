/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/inline_content_ad/catalog_inline_content_ad_payload_info.h"

namespace brave_ads {

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

}  // namespace brave_ads
