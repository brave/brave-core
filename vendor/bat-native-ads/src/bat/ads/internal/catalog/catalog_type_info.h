/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_TYPE_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_TYPE_INFO_H_

#include <cstdint>
#include <string>

namespace ads {

struct CatalogTypeInfo final {
  CatalogTypeInfo();
  CatalogTypeInfo(const CatalogTypeInfo& info);
  ~CatalogTypeInfo();

  bool operator==(const CatalogTypeInfo& rhs) const;
  bool operator!=(const CatalogTypeInfo& rhs) const;

  std::string code;
  std::string name;
  std::string platform;
  uint64_t version = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_TYPE_INFO_H_
