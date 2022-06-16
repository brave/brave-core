/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/catalog_geo_target_info.h"

namespace ads {

CatalogGeoTargetInfo::CatalogGeoTargetInfo() = default;

CatalogGeoTargetInfo::CatalogGeoTargetInfo(const CatalogGeoTargetInfo& info) =
    default;

CatalogGeoTargetInfo& CatalogGeoTargetInfo::operator=(
    const CatalogGeoTargetInfo& info) = default;

CatalogGeoTargetInfo::~CatalogGeoTargetInfo() = default;

bool CatalogGeoTargetInfo::operator==(const CatalogGeoTargetInfo& rhs) const {
  return code == rhs.code && name == rhs.name;
}

bool CatalogGeoTargetInfo::operator!=(const CatalogGeoTargetInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
