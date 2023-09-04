/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_os_info.h"

#include <tuple>

namespace brave_ads {

bool operator==(const CatalogOsInfo& lhs, const CatalogOsInfo& rhs) {
  const auto tie = [](const CatalogOsInfo& os) {
    return std::tie(os.code, os.name);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const CatalogOsInfo& lhs, const CatalogOsInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
