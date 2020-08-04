/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_CATALOG_ISSUER_INFO_H_
#define BAT_ADS_CATALOG_ISSUER_INFO_H_

#include <string>
#include <vector>

namespace ads {

struct CatalogIssuerInfo {
  CatalogIssuerInfo();
  CatalogIssuerInfo(
      const CatalogIssuerInfo& info);
  ~CatalogIssuerInfo();

  std::string name;
  std::string public_key;
};

using CatalogIssuerList = std::vector<CatalogIssuerInfo>;

}  // namespace ads

#endif  // BAT_ADS_CATALOG_ISSUER_INFO_H_
