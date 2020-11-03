/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_TABS_TABS_H_
#define BAT_ADS_INTERNAL_TABS_TABS_H_

#include <stdint.h>

#include <map>
#include <string>

#include "bat/ads/internal/tabs/tab_info.h"

namespace ads {

class AdsImpl;

class Tabs {
 public:
  Tabs(
    AdsImpl* ads);

  ~Tabs();

  bool IsVisible(
      const int32_t id) const;

  void OnUpdated(
      const int32_t id,
      const std::string& url,
      const bool is_visible,
      const bool is_incognito);

  void OnClosed(
      const int32_t id);

  void OnMediaPlaying(
      const int32_t id);
  void OnMediaStopped(
      const int32_t id);

  bool IsPlayingMedia(
      const int32_t id) const;

  TabInfo GetVisible() const;

  TabInfo GetLastVisible() const;

 private:
  int32_t visible_tab_id_ = 0;
  int32_t last_visible_tab_id_ = 0;

  std::map<int32_t, TabInfo> tabs_;

  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_TABS_TABS_H_
