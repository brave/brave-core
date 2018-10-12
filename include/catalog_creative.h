/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>

#include "../include/catalog_type.h"
#include "../include/catalog_payload.h"

namespace catalog {

struct CreativeInfo {
  CreativeInfo() :
      creative_id("") {}

  CreativeInfo(const CreativeInfo& info) :
      creative_id(info.creative_id) {}

  ~CreativeInfo() {}

  std::string creative_id;
  std::vector<TypeInfo> types;
  std::vector<PayloadInfo> payloads;
};

}  // namespace catalog
