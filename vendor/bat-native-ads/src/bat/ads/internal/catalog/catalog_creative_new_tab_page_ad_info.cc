/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_creative_new_tab_page_ad_info.h"

namespace ads {

CatalogCreativeNewTabPageAdInfo::CatalogCreativeNewTabPageAdInfo() = default;

CatalogCreativeNewTabPageAdInfo::CatalogCreativeNewTabPageAdInfo(
    const CatalogCreativeNewTabPageAdInfo& info) = default;

CatalogCreativeNewTabPageAdInfo::~CatalogCreativeNewTabPageAdInfo() = default;

bool CatalogCreativeNewTabPageAdInfo::operator==(
    const CatalogCreativeNewTabPageAdInfo& rhs) const {
  return payload == rhs.payload;
}

bool CatalogCreativeNewTabPageAdInfo::operator!=(
    const CatalogCreativeNewTabPageAdInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
