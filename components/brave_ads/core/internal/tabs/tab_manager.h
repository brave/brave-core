/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_TAB_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_TAB_MANAGER_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class GURL;

namespace brave_ads {

class TabManager : public AdsClientNotifierObserver {
 public:
  TabManager();

  TabManager(const TabManager& other) = delete;
  TabManager& operator=(const TabManager& other) = delete;

  TabManager(TabManager&& other) noexcept = delete;
  TabManager& operator=(TabManager&& other) noexcept = delete;

  ~TabManager() override;

  static TabManager* GetInstance();

  static bool HasInstance();

  void AddObserver(TabManagerObserver* observer);
  void RemoveObserver(TabManagerObserver* observer);

  bool IsVisible(int32_t id) const;

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

  // AdsClientNotifierObserver:
  void OnNotifyTabHtmlContentDidChange(int32_t id,
                                       const std::vector<GURL>& redirect_chain,
                                       const std::string& content) override;
  void OnNotifyTabTextContentDidChange(int32_t id,
                                       const std::vector<GURL>& redirect_chain,
                                       const std::string& content) override;
  void OnNotifyTabDidStartPlayingMedia(int32_t id) override;
  void OnNotifyTabDidStopPlayingMedia(int32_t id) override;
  void OnNotifyTabDidChange(int32_t id,
                            const std::vector<GURL>& redirect_chain,
                            bool is_visible,
                            bool is_incognito) override;
  void OnNotifyDidCloseTab(int32_t id) override;

  base::ObserverList<TabManagerObserver> observers_;

  uint32_t last_text_content_hash_ = 0;
  uint32_t last_html_content_hash_ = 0;

  int32_t visible_tab_id_ = 0;
  int32_t last_visible_tab_id_ = 0;

  std::map<int32_t, TabInfo> tabs_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_TAB_MANAGER_H_
