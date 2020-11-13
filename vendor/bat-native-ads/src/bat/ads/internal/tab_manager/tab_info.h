/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_TAB_MANAGER_TAB_INFO_H_
#define BAT_ADS_INTERNAL_TAB_MANAGER_TAB_INFO_H_

#include <stdint.h>

#include <string>

namespace ads {

struct TabInfo {
  TabInfo();
  TabInfo(
      const TabInfo& info);
  ~TabInfo();

  int32_t id = 0;
  std::string url;
  bool is_playing_media = false;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_TAB_MANAGER_TAB_INFO_H_
