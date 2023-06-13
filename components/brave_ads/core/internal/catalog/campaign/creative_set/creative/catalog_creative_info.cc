/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/catalog_creative_info.h"

namespace brave_ads {

bool CatalogCreativeInfo::operator==(const CatalogCreativeInfo& other) const {
  return instance_id == other.instance_id && type == other.type;
}

bool CatalogCreativeInfo::operator!=(const CatalogCreativeInfo& other) const {
  return !(*this == other);
}

}  // namespace brave_ads
