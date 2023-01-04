/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TABS_TAB_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TABS_TAB_INFO_H_

#include <cstdint>
#include <vector>

#include "url/gurl.h"

namespace ads {

struct TabInfo final {
  TabInfo();

  TabInfo(const TabInfo& other);
  TabInfo& operator=(const TabInfo& other);

  TabInfo(TabInfo&& other) noexcept;
  TabInfo& operator=(TabInfo&& other) noexcept;

  ~TabInfo();

  bool operator==(const TabInfo& other) const;
  bool operator!=(const TabInfo& other) const;

  int32_t id = 0;
  std::vector<GURL> redirect_chain;
  bool is_playing_media = false;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TABS_TAB_INFO_H_
