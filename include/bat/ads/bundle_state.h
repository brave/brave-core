/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>
#include <map>

#include "bat/ads/bundle_category_info.h"
#include "json_helper.h"

namespace ads {

struct BUNDLE_STATE {
  BUNDLE_STATE();
  explicit BUNDLE_STATE(const BUNDLE_STATE& state);
  ~BUNDLE_STATE();

  bool LoadFromJson(const std::string& json);

  bool validateJson(
      const rapidjson::Document& document,
      const std::map<std::string, std::string>& members);

  std::string catalog_id;
  uint64_t catalog_version;
  uint64_t catalog_ping;
  std::map<std::string, std::vector<bundle::CategoryInfo>> categories;
};

}  // namespace ads
