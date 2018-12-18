/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_EVENT_TYPE_LOAD_INFO_H_
#define BAT_ADS_EVENT_TYPE_LOAD_INFO_H_

#include <string>

namespace ads {

struct LoadInfo {
  LoadInfo() :
      tab_id(-1),
      tab_url("") {}

  explicit LoadInfo(const LoadInfo& info) :
      tab_id(info.tab_id),
      tab_url(info.tab_url) {}

  ~LoadInfo() {}

  int32_t tab_id;
  std::string tab_url;
};

}  // namespace ads

#endif  // BAT_ADS_EVENT_TYPE_LOAD_INFO_H_
