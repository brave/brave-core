/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_TAB_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_TAB_INFO_H_

#include <cstdint>
#include <vector>

#include "url/gurl.h"

namespace brave_ads {

struct TabInfo final {
  TabInfo();
  TabInfo(int32_t id,
          bool is_visible,
          std::vector<GURL> redirect_chain,
          bool is_playing_media);

  TabInfo(const TabInfo&);
  TabInfo& operator=(const TabInfo&);

  TabInfo(TabInfo&&) noexcept;
  TabInfo& operator=(TabInfo&&) noexcept;

  ~TabInfo();

  bool operator==(const TabInfo&) const = default;

  int32_t id = 0;
  bool is_visible = false;
  std::vector<GURL> redirect_chain;
  bool is_playing_media = false;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_TAB_INFO_H_
