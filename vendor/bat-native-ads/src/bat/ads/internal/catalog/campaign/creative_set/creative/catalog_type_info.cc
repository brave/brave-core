/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/creative_set/creative/catalog_type_info.h"

namespace ads {

CatalogTypeInfo::CatalogTypeInfo() = default;

CatalogTypeInfo::CatalogTypeInfo(const CatalogTypeInfo& info) = default;

CatalogTypeInfo& CatalogTypeInfo::operator=(const CatalogTypeInfo& info) =
    default;

CatalogTypeInfo::~CatalogTypeInfo() = default;

bool CatalogTypeInfo::operator==(const CatalogTypeInfo& rhs) const {
  return code == rhs.code && name == rhs.name && platform == rhs.platform &&
         version == rhs.version;
}

bool CatalogTypeInfo::operator!=(const CatalogTypeInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
