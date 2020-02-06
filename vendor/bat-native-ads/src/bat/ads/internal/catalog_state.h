/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CATALOG_STATE_H_
#define BAT_ADS_INTERNAL_CATALOG_STATE_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>

#include "bat/ads/issuers_info.h"

#include "bat/ads/internal/catalog_campaign_info.h"
#include "bat/ads/internal/json_helper.h"

namespace ads {

struct CatalogState {
  CatalogState();
  CatalogState(
      const CatalogState& state);
  ~CatalogState();

  Result FromJson(
      const std::string& json,
      const std::string& json_schema,
      std::string* error_description = nullptr);

  std::string catalog_id;
  uint64_t version = 0;
  uint64_t ping = 0;
  std::vector<CatalogCampaignInfo> campaigns;
  IssuersInfo issuers;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CATALOG_STATE_H_
