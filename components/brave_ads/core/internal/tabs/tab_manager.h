/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_TAB_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_TAB_MANAGER_H_

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

class GURL;

namespace brave_ads {

class TabManager final : public AdsClientNotifierObserver {
 public:
  TabManager();

  TabManager(const TabManager&) = delete;
  TabManager& operator=(const TabManager&) = delete;

  TabManager(TabManager&&) noexcept = delete;
  TabManager& operator=(TabManager&&) noexcept = delete;

  ~TabManager() override;

  static TabManager& GetInstance();

  void AddObserver(TabManagerObserver* observer);
  void RemoveObserver(TabManagerObserver* observer);

  bool IsVisible(int32_t tab_id) const { return visible_tab_id_ == tab_id; }
  std::optional<TabInfo> MaybeGetVisible() const;

  std::optional<TabInfo> MaybeGetForId(int32_t tab_id) const;

  bool IsPlayingMedia(int32_t tab_id) const;

 private:
  bool DoesExistForId(int32_t tab_id) const;
  TabInfo& GetOrCreateForId(int32_t tab_id);
  void RemoveForId(int32_t tab_id);

  void NotifyTabDidChangeFocus(int32_t tab_id) const;
  void NotifyTabDidChange(const TabInfo& tab) const;
  void NotifyTabDidLoad(const TabInfo& tab, int http_status_code) const;
  void NotifyDidOpenNewTab(const TabInfo& tab) const;
  void NotifyTextContentDidChange(int32_t tab_id,
                                  const std::vector<GURL>& redirect_chain,
                                  const std::string& text);
  void NotifyHtmlContentDidChange(int32_t tab_id,
                                  const std::vector<GURL>& redirect_chain,
                                  const std::string& html);
  void NotifyDidCloseTab(int32_t tab_id) const;
  void NotifyTabDidStartPlayingMedia(int32_t tab_id) const;
  void NotifyTabDidStopPlayingMedia(int32_t tab_id) const;

  // AdsClientNotifierObserver:
  void OnNotifyTabTextContentDidChange(int32_t tab_id,
                                       const std::vector<GURL>& redirect_chain,
                                       const std::string& text) override;
  void OnNotifyTabHtmlContentDidChange(int32_t tab_id,
                                       const std::vector<GURL>& redirect_chain,
                                       const std::string& html) override;
  void OnNotifyTabDidStartPlayingMedia(int32_t tab_id) override;
  void OnNotifyTabDidStopPlayingMedia(int32_t tab_id) override;
  void OnNotifyTabDidChange(int32_t tab_id,
                            const std::vector<GURL>& redirect_chain,
                            bool is_new_navigation,
                            bool is_restoring,
                            bool is_visible) override;
  void OnNotifyTabDidLoad(int32_t tab_id, int http_status_code) override;
  void OnNotifyDidCloseTab(int32_t tab_id) override;

  base::ObserverList<TabManagerObserver> observers_;

  std::optional<int32_t> visible_tab_id_;

  std::map</*tab_id*/ int32_t, TabInfo> tabs_;

  uint32_t last_text_content_hash_ = 0;
  uint32_t last_html_content_hash_ = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_TAB_MANAGER_H_
