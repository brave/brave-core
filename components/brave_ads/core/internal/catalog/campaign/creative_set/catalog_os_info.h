/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_OS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_OS_INFO_H_

#include <string>
#include <vector>

namespace brave_ads {

struct CatalogOsInfo final {
  bool operator==(const CatalogOsInfo&) const = default;

  std::string code;
  std::string name;
};

using CatalogOsList = std::vector<CatalogOsInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_OS_INFO_H_
