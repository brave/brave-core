/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_STATE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_STATE_H_

#include <cstdint>
#include <string>

#include "bat/ads/internal/catalog/catalog_campaign_info.h"
#include "bat/ads/internal/catalog/catalog_issuers_info.h"

namespace ads {

struct CatalogState {
  CatalogState();
  CatalogState(const CatalogState& state);
  ~CatalogState();

  bool FromJson(const std::string& json, const std::string& json_schema);

  std::string catalog_id;
  int version = 0;
  int64_t ping = 0;
  CatalogCampaignList campaigns;
  CatalogIssuersInfo catalog_issuers;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_STATE_H_
