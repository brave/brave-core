/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>
#include <map>

#include "catalog_campaign_info.h"
#include "json_helper.h"

namespace ads {

struct CATALOG_STATE {
  CATALOG_STATE();
  explicit CATALOG_STATE(const CATALOG_STATE& state);
  ~CATALOG_STATE();

  bool LoadFromJson(const std::string& json, const std::string& jsonSchema);

  bool validateJson(
      const rapidjson::Document& document,
      const std::map<std::string, std::string>& members);

  std::string catalog_id;
  uint64_t version;
  uint64_t ping;
  std::vector<CampaignInfo> campaigns;
};

}  // namespace ads
