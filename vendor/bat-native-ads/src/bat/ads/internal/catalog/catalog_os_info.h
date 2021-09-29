/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_OS_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_OS_INFO_H_

#include <string>

namespace ads {

struct CatalogOsInfo final {
  CatalogOsInfo();
  CatalogOsInfo(const CatalogOsInfo& info);
  ~CatalogOsInfo();

  bool operator==(const CatalogOsInfo& rhs) const;
  bool operator!=(const CatalogOsInfo& rhs) const;

  std::string code;
  std::string name;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_OS_INFO_H_
