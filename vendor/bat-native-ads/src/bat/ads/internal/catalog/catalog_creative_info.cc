/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_creative_info.h"

namespace ads {

CatalogCreativeInfo::CatalogCreativeInfo() = default;

CatalogCreativeInfo::CatalogCreativeInfo(
    const CatalogCreativeInfo& info) = default;

CatalogCreativeInfo::~CatalogCreativeInfo() = default;

bool CatalogCreativeInfo::operator==(
    const CatalogCreativeInfo& rhs) const {
  return creative_instance_id == rhs.creative_instance_id &&
      type == rhs.type;
}

bool CatalogCreativeInfo::operator!=(
    const CatalogCreativeInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
