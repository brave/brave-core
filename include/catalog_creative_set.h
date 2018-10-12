/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>

#include "../include/catalog_segment.h"
#include "../include/catalog_creative.h"

namespace catalog {

struct CreativeSetInfo {
  CreativeSetInfo() :
      creative_set_id(""),
      execution(""),
      per_day(0),
      total_max(0) {}

  explicit CreativeSetInfo(const std::string& creative_set_id) :
      creative_set_id(creative_set_id),
      execution(""),
      per_day(0),
      total_max(0) {}

  CreativeSetInfo(const CreativeSetInfo& info) :
      creative_set_id(info.creative_set_id),
      execution(info.execution),
      per_day(info.per_day),
      total_max(info.total_max) {}

  ~CreativeSetInfo() {}

  std::string creative_set_id;
  std::string execution;
  int64_t per_day;
  int64_t total_max;
  std::vector<SegmentInfo> segments;
  std::vector<CreativeInfo> creatives;
};

struct CreativeSetInfoFilter {
  CreativeSetInfoFilter() :
      creative_set_id("") {}

  CreativeSetInfoFilter(const CreativeSetInfoFilter& filter) :
      creative_set_id(filter.creative_set_id) {}

  ~CreativeSetInfoFilter() {}

  std::string creative_set_id;
};

}  // namespace catalog
