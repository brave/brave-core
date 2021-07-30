/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TAB_MANAGER_TAB_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TAB_MANAGER_TAB_MANAGER_H_

#include <cstdint>
#include <map>
#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

struct TabInfo;

class TabManager {
 public:
  TabManager();

  ~TabManager();

  TabManager(const TabManager&) = delete;
  TabManager& operator=(const TabManager&) = delete;

  static TabManager* Get();

  static bool HasInstance();

  bool IsVisible(const int32_t id) const;

  void OnUpdated(const int32_t id,
                 const std::string& url,
                 const bool is_visible,
                 const bool is_incognito);

  void OnClosed(const int32_t id);

  void OnMediaPlaying(const int32_t id);
  void OnMediaStopped(const int32_t id);

  bool IsPlayingMedia(const int32_t id) const;

  absl::optional<TabInfo> GetVisible() const;

  absl::optional<TabInfo> GetLastVisible() const;

  absl::optional<TabInfo> GetForId(const int32_t id) const;

 private:
  int32_t visible_tab_id_ = 0;
  int32_t last_visible_tab_id_ = 0;

  std::map<int32_t, TabInfo> tabs_;

  void AddTab(const int32_t id, const TabInfo& tab);
  void UpdateTab(const int32_t id, const TabInfo& tab);
  void RemoveTab(const int32_t id);
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TAB_MANAGER_TAB_MANAGER_H_
