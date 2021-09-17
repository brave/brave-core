/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_NEW_TAB_PAGE_AD_PAYLOAD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_NEW_TAB_PAGE_AD_PAYLOAD_INFO_H_

#include <string>

namespace ads {

struct CatalogNewTabPageAdPayloadInfo final {
  CatalogNewTabPageAdPayloadInfo();
  CatalogNewTabPageAdPayloadInfo(const CatalogNewTabPageAdPayloadInfo& info);
  ~CatalogNewTabPageAdPayloadInfo();

  bool operator==(const CatalogNewTabPageAdPayloadInfo& rhs) const;
  bool operator!=(const CatalogNewTabPageAdPayloadInfo& rhs) const;

  std::string company_name;
  std::string alt;
  std::string target_url;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_NEW_TAB_PAGE_AD_PAYLOAD_INFO_H_
