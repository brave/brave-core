/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_INFO_H_

#include <cstdint>
#include <string>

#include "bat/ads/internal/catalog/catalog_campaign_info_aliases.h"

namespace ads {

struct CatalogInfo final {
  CatalogInfo();
  CatalogInfo(const CatalogInfo& info);
  ~CatalogInfo();

  bool FromJson(const std::string& json, const std::string& json_schema);

  std::string id;
  int version = 0;
  int64_t ping = 0;
  CatalogCampaignList campaigns;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_INFO_H_
