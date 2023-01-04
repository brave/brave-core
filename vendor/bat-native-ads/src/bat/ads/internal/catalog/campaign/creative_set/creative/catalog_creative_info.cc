/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/creative_set/creative/catalog_creative_info.h"

namespace ads {

bool CatalogCreativeInfo::operator==(const CatalogCreativeInfo& other) const {
  return creative_instance_id == other.creative_instance_id &&
         type == other.type;
}

bool CatalogCreativeInfo::operator!=(const CatalogCreativeInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
