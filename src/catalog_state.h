/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>
#include <map>

#include "campaign_info.h"
#include "json_helper.h"

namespace ads {

struct CatalogState {
  CatalogState();
  explicit CatalogState(const CatalogState& state);
  ~CatalogState();

  bool FromJson(const std::string& json, const std::string& jsonSchema);

  std::string catalog_id;
  uint64_t version;
  uint64_t ping;
  std::vector<CampaignInfo> campaigns;
};

}  // namespace ads
