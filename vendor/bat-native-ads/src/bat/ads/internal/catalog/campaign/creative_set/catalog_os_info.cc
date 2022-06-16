/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/creative_set/catalog_os_info.h"

namespace ads {

CatalogOsInfo::CatalogOsInfo() = default;

CatalogOsInfo::CatalogOsInfo(const CatalogOsInfo& info) = default;

CatalogOsInfo& CatalogOsInfo::operator=(const CatalogOsInfo& info) = default;

CatalogOsInfo::~CatalogOsInfo() = default;

bool CatalogOsInfo::operator==(const CatalogOsInfo& rhs) const {
  return code == rhs.code && name == rhs.name;
}

bool CatalogOsInfo::operator!=(const CatalogOsInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
