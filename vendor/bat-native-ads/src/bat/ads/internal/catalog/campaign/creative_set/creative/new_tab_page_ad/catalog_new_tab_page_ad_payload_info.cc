/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/creative_set/creative/new_tab_page_ad/catalog_new_tab_page_ad_payload_info.h"

namespace ads {

CatalogNewTabPageAdPayloadInfo::CatalogNewTabPageAdPayloadInfo() = default;

CatalogNewTabPageAdPayloadInfo::CatalogNewTabPageAdPayloadInfo(
    const CatalogNewTabPageAdPayloadInfo& other) = default;

CatalogNewTabPageAdPayloadInfo& CatalogNewTabPageAdPayloadInfo::operator=(
    const CatalogNewTabPageAdPayloadInfo& other) = default;

CatalogNewTabPageAdPayloadInfo::CatalogNewTabPageAdPayloadInfo(
    CatalogNewTabPageAdPayloadInfo&& other) noexcept = default;

CatalogNewTabPageAdPayloadInfo& CatalogNewTabPageAdPayloadInfo::operator=(
    CatalogNewTabPageAdPayloadInfo&& other) noexcept = default;

CatalogNewTabPageAdPayloadInfo::~CatalogNewTabPageAdPayloadInfo() = default;

bool CatalogNewTabPageAdPayloadInfo::operator==(
    const CatalogNewTabPageAdPayloadInfo& other) const {
  return company_name == other.company_name && image_url == other.image_url &&
         alt == other.alt && target_url == other.target_url &&
         wallpapers == other.wallpapers;
}

bool CatalogNewTabPageAdPayloadInfo::operator!=(
    const CatalogNewTabPageAdPayloadInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
