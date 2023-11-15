/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/new_tab_page_ad/catalog_new_tab_page_ad_payload_info.h"

namespace brave_ads {

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

}  // namespace brave_ads
