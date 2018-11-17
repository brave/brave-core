/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

namespace ads {

struct BlurInfo {
  BlurInfo() :
      tab_id(-1) {}

  explicit BlurInfo(const BlurInfo& info) :
      tab_id(info.tab_id) {}

  ~BlurInfo() {}

  int32_t tab_id;
};

}  // namespace ads
