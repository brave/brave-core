/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>
#include <map>

#include "bat/ads/ad_info.h"

namespace ads {

struct BUNDLE_STATE {
  BUNDLE_STATE();
  explicit BUNDLE_STATE(const BUNDLE_STATE& state);
  ~BUNDLE_STATE();

  bool LoadFromJson(const std::string& json, const std::string& jsonSchema);

  std::string catalog_id;
  uint64_t catalog_version;
  uint64_t catalog_ping;
  std::map<std::string, std::vector<AdInfo>> categories;
};

}  // namespace ads
