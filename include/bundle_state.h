/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>
#include <map>

#include "bundle_category_info.h"
#include "json_helper.h"

namespace state {

struct BUNDLE_STATE {
  BUNDLE_STATE();
  explicit BUNDLE_STATE(const BUNDLE_STATE& state);
  ~BUNDLE_STATE();

  bool LoadFromJson(const std::string& json);

  bool validateJson(
      const rapidjson::Document& document,
      const std::map<std::string, std::string>& members);

  std::map<std::string, std::vector<bundle::CategoryInfo>> categories;
};

}  // namespace state
