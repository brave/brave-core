/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_CATALOG_TYPE_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_CATALOG_TYPE_INFO_H_

#include <string>

namespace brave_ads {

struct CatalogTypeInfo final {
  CatalogTypeInfo();

  CatalogTypeInfo(const CatalogTypeInfo&);
  CatalogTypeInfo& operator=(const CatalogTypeInfo&);

  CatalogTypeInfo(CatalogTypeInfo&&) noexcept;
  CatalogTypeInfo& operator=(CatalogTypeInfo&&) noexcept;

  ~CatalogTypeInfo();

  bool operator==(const CatalogTypeInfo&) const = default;

  std::string code;
  std::string name;
  std::string platform;
  int version = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_CATALOG_TYPE_INFO_H_
