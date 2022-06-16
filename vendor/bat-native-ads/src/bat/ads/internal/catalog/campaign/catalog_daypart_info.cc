/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/catalog_daypart_info.h"

namespace ads {

CatalogDaypartInfo::CatalogDaypartInfo() = default;

CatalogDaypartInfo::CatalogDaypartInfo(const CatalogDaypartInfo& info) =
    default;

CatalogDaypartInfo& CatalogDaypartInfo::operator=(
    const CatalogDaypartInfo& info) = default;

CatalogDaypartInfo::~CatalogDaypartInfo() = default;

bool CatalogDaypartInfo::operator==(const CatalogDaypartInfo& rhs) const {
  return dow == rhs.dow && start_minute == rhs.start_minute &&
         end_minute == rhs.end_minute;
}

bool CatalogDaypartInfo::operator!=(const CatalogDaypartInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
