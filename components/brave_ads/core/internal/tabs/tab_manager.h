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
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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

  bool IsVisible(int32_t tab_id) const;

  bool IsPlayingMedia(int32_t tab_id) const;

  absl::optional<TabInfo> GetVisible() const;
  absl::optional<TabInfo> GetLastVisible() const;

  absl::optional<TabInfo> MaybeGetForId(int32_t tab_id) const;

 private:
  TabInfo& GetOrCreateForId(int32_t tab_id);
  void Remove(int32_t tab_id);

  void NotifyTabDidChangeFocus(int32_t tab_id) const;
  void NotifyTabDidChange(const TabInfo& tab) const;
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
                            bool is_visible) override;
  void OnNotifyDidCloseTab(int32_t tab_id) override;

  base::ObserverList<TabManagerObserver> observers_;

  uint32_t last_text_content_hash_ = 0;
  uint32_t last_html_content_hash_ = 0;

  absl::optional<int32_t> visible_tab_id_;
  absl::optional<int32_t> last_visible_tab_id_;

  std::map</*tab_id=*/int32_t, TabInfo> tabs_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_TAB_MANAGER_H_
