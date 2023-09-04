/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/inline_content_ad/catalog_inline_content_ad_payload_info.h"

#include <tuple>

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

bool CatalogInlineContentAdPayloadInfo::operator==(
    const CatalogInlineContentAdPayloadInfo& other) const {
  const auto tie = [](const CatalogInlineContentAdPayloadInfo& payload) {
    return std::tie(payload.title, payload.description, payload.image_url,
                    payload.dimensions, payload.cta_text, payload.target_url);
  };

  return tie(*this) == tie(other);
}

bool CatalogInlineContentAdPayloadInfo::operator!=(
    const CatalogInlineContentAdPayloadInfo& other) const {
  return !(*this == other);
}

}  // namespace brave_ads
