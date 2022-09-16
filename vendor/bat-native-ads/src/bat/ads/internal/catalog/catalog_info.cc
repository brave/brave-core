/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_info.h"

namespace ads {

CatalogInfo::CatalogInfo() = default;

CatalogInfo::CatalogInfo(const CatalogInfo& info) = default;

CatalogInfo& CatalogInfo::operator=(const CatalogInfo& info) = default;

CatalogInfo::CatalogInfo(CatalogInfo&& other) noexcept = default;

CatalogInfo& CatalogInfo::operator=(CatalogInfo&& other) noexcept = default;

CatalogInfo::~CatalogInfo() = default;

bool CatalogInfo::operator==(const CatalogInfo& rhs) const {
  return id == rhs.id && version == rhs.version && ping == rhs.ping &&
         campaigns == rhs.campaigns;
}

bool CatalogInfo::operator!=(const CatalogInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
