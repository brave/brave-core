/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_SEARCH_RESULT_AD_PAYLOAD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_SEARCH_RESULT_AD_PAYLOAD_INFO_H_

#include <string>

namespace ads {

struct CatalogSearchResultAdPayloadInfo final {
  CatalogSearchResultAdPayloadInfo();
  CatalogSearchResultAdPayloadInfo(
      const CatalogSearchResultAdPayloadInfo& info);
  ~CatalogSearchResultAdPayloadInfo();

  bool operator==(const CatalogSearchResultAdPayloadInfo& rhs) const;
  bool operator!=(const CatalogSearchResultAdPayloadInfo& rhs) const;

  std::string title;
  std::string body;
  std::string target_url;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_SEARCH_RESULT_AD_PAYLOAD_INFO_H_
