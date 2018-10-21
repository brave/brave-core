/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

namespace catalog {

struct SegmentInfo {
  SegmentInfo() :
    code(""),
    name("") {}

  explicit SegmentInfo(const SegmentInfo& info) :
    code(info.code),
    name(info.name) {}

  ~SegmentInfo() {}

  std::string code;
  std::string name;
};

}  // namespace catalog
