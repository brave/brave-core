/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_new_tab_page_ad_payload_info.h"

namespace ads {

CatalogNewTabPageAdPayloadInfo::CatalogNewTabPageAdPayloadInfo() = default;

CatalogNewTabPageAdPayloadInfo::CatalogNewTabPageAdPayloadInfo(
    const CatalogNewTabPageAdPayloadInfo& info) = default;

CatalogNewTabPageAdPayloadInfo::~CatalogNewTabPageAdPayloadInfo() = default;

bool CatalogNewTabPageAdPayloadInfo::operator==(
    const CatalogNewTabPageAdPayloadInfo& rhs) const {
  return company_name == rhs.company_name &&
      alt == rhs.alt &&
      target_url == rhs.target_url;
}

bool CatalogNewTabPageAdPayloadInfo::operator!=(
    const CatalogNewTabPageAdPayloadInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
