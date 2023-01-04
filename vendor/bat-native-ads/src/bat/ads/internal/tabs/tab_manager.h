/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TABS_TAB_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TABS_TAB_MANAGER_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "absl/types/optional.h"
#include "base/observer_list.h"
#include "bat/ads/internal/tabs/tab_info.h"
#include "bat/ads/internal/tabs/tab_manager_observer.h"

class GURL;

namespace ads {

class TabManager final {
 public:
  TabManager();

  TabManager(const TabManager& other) = delete;
  TabManager& operator=(const TabManager& other) = delete;

  TabManager(TabManager&& other) noexcept = delete;
  TabManager& operator=(TabManager&& other) noexcept = delete;

  ~TabManager();

  static TabManager* GetInstance();

  static bool HasInstance();

  void AddObserver(TabManagerObserver* observer);
  void RemoveObserver(TabManagerObserver* observer);

  bool IsVisible(int32_t id) const;

  void OnDidChange(int32_t id,
                   const std::vector<GURL>& redirect_chain,
                   bool is_visible,
                   bool is_incognito);

  void OnTextContentDidChange(int32_t id,
                              const std::vector<GURL>& redirect_chain,
                              const std::string& content);
  void OnHtmlContentDidChange(int32_t id,
                              const std::vector<GURL>& redirect_chain,
                              const std::string& content);

  void OnDidClose(int32_t id);

  void OnDidStartPlayingMedia(int32_t id);
  void OnDidStopPlayingMedia(int32_t id);

  bool IsPlayingMedia(int32_t id) const;

  absl::optional<TabInfo> GetVisible() const;
  absl::optional<TabInfo> GetLastVisible() const;

  absl::optional<TabInfo> GetForId(int32_t id) const;

 private:
  void Add(const TabInfo& tab);
  void Update(const TabInfo& tab);
  void Remove(int32_t id);

  void NotifyTabDidChangeFocus(int32_t id) const;
  void NotifyTabDidChange(const TabInfo& tab) const;
  void NotifyDidOpenNewTab(const TabInfo& tab) const;
  void NotifyTextContentDidChange(int32_t id,
                                  const std::vector<GURL>& redirect_chain,
                                  const std::string& content);
  void NotifyHtmlContentDidChange(int32_t id,
                                  const std::vector<GURL>& redirect_chain,
                                  const std::string& content);
  void NotifyDidCloseTab(int32_t id) const;
  void NotifyTabDidStartPlayingMedia(int32_t id) const;
  void NotifyTabDidStopPlayingMedia(int32_t id) const;

  base::ObserverList<TabManagerObserver> observers_;

  uint32_t last_text_content_hash_ = 0;
  uint32_t last_html_content_hash_ = 0;

  int32_t visible_tab_id_ = 0;
  int32_t last_visible_tab_id_ = 0;

  std::map<int32_t, TabInfo> tabs_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TABS_TAB_MANAGER_H_
