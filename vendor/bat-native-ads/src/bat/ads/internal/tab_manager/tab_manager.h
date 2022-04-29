/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TAB_MANAGER_TAB_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TAB_MANAGER_TAB_MANAGER_H_

#include <cstdint>
#include <map>

#include "base/observer_list.h"
#include "bat/ads/internal/tab_manager/tab_info.h"
#include "bat/ads/internal/tab_manager/tab_manager_observer.h"

class GURL;

namespace absl {
template <typename T>
class optional;
}  // namespace absl

namespace ads {

class TabManager final {
 public:
  TabManager();
  ~TabManager();

  TabManager(const TabManager&) = delete;
  TabManager& operator=(const TabManager&) = delete;

  static TabManager* Get();

  static bool HasInstance();

  void AddObserver(TabManagerObserver* observer);
  void RemoveObserver(TabManagerObserver* observer);

  bool IsVisible(const int32_t id) const;

  void OnUpdated(const int32_t id,
                 const GURL& url,
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
  void AddTab(const int32_t id, const TabInfo& tab);
  void UpdateTab(const int32_t id, const TabInfo& tab);
  void RemoveTab(const int32_t id);

  void NotifyTabDidChangeFocus(const int32_t id) const;
  void NotifyTabDidChange(const int32_t id) const;
  void NotifyDidOpenNewTab(const int32_t id) const;
  void NotifyDidCloseTab(const int32_t id) const;
  void NotifyTabDidStartPlayingMedia(const int32_t id) const;
  void NotifyTabDidStopPlayingMedia(const int32_t id) const;

  base::ObserverList<TabManagerObserver> observers_;

  int32_t visible_tab_id_ = 0;
  int32_t last_visible_tab_id_ = 0;

  std::map<int32_t, TabInfo> tabs_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TAB_MANAGER_TAB_MANAGER_H_
