/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_EVENT_TYPE_BLUR_INFO_H_
#define BAT_ADS_EVENT_TYPE_BLUR_INFO_H_

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

#endif  // BAT_ADS_EVENT_TYPE_BLUR_INFO_H_
