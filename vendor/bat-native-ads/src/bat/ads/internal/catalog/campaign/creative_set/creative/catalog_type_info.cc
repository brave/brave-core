/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/creative_set/creative/catalog_type_info.h"

namespace ads {

CatalogTypeInfo::CatalogTypeInfo() = default;

CatalogTypeInfo::CatalogTypeInfo(const CatalogTypeInfo& other) = default;

CatalogTypeInfo& CatalogTypeInfo::operator=(const CatalogTypeInfo& other) =
    default;

CatalogTypeInfo::CatalogTypeInfo(CatalogTypeInfo&& other) noexcept = default;

CatalogTypeInfo& CatalogTypeInfo::operator=(CatalogTypeInfo&& other) noexcept =
    default;

CatalogTypeInfo::~CatalogTypeInfo() = default;

bool CatalogTypeInfo::operator==(const CatalogTypeInfo& other) const {
  return code == other.code && name == other.name &&
         platform == other.platform && version == other.version;
}

bool CatalogTypeInfo::operator!=(const CatalogTypeInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
