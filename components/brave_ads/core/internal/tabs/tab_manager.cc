/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"

#include "base/check.h"
#include "base/hash/hash.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "url/gurl.h"

namespace brave_ads {

TabManager::TabManager() {
  AdsClientHelper::AddObserver(this);
}

TabManager::~TabManager() {
  AdsClientHelper::RemoveObserver(this);
}

// static
TabManager& TabManager::GetInstance() {
  return GlobalState::GetInstance()->GetTabManager();
}

void TabManager::AddObserver(TabManagerObserver* observer) {
  CHECK(observer);
  observers_.AddObserver(observer);
}

void TabManager::RemoveObserver(TabManagerObserver* observer) {
  CHECK(observer);
  observers_.RemoveObserver(observer);
}

bool TabManager::IsVisible(const int32_t id) const {
  if (id == 0) {
    return false;
  }

  return visible_tab_id_ == id;
}

bool TabManager::IsPlayingMedia(const int32_t id) const {
  const absl::optional<TabInfo> tab = GetForId(id);
  if (!tab) {
    return false;
  }

  return tab->is_playing_media;
}

absl::optional<TabInfo> TabManager::GetVisible() const {
  return GetForId(visible_tab_id_);
}

absl::optional<TabInfo> TabManager::GetLastVisible() const {
  return GetForId(last_visible_tab_id_);
}

absl::optional<TabInfo> TabManager::GetForId(const int32_t id) const {
  if (tabs_.find(id) == tabs_.cend()) {
    return absl::nullopt;
  }

  return tabs_.at(id);
}

///////////////////////////////////////////////////////////////////////////////

void TabManager::Add(const TabInfo& tab) {
  CHECK(!GetForId(tab.id));
  tabs_[tab.id] = tab;
}

void TabManager::Update(const TabInfo& tab) {
  CHECK(GetForId(tab.id));
  tabs_[tab.id] = tab;
}

void TabManager::Remove(const int32_t id) {
  tabs_.erase(id);
}

void TabManager::NotifyTabDidChangeFocus(const int32_t id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTabDidChangeFocus(id);
  }
}

void TabManager::NotifyTabDidChange(const TabInfo& tab) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTabDidChange(tab);
  }
}

void TabManager::NotifyDidOpenNewTab(const TabInfo& tab) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnDidOpenNewTab(tab);
  }
}

void TabManager::NotifyTextContentDidChange(
    const int32_t id,
    const std::vector<GURL>& redirect_chain,
    const std::string& content) {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTextContentDidChange(id, redirect_chain, content);
  }
}

void TabManager::NotifyHtmlContentDidChange(
    const int32_t id,
    const std::vector<GURL>& redirect_chain,
    const std::string& content) {
  for (TabManagerObserver& observer : observers_) {
    observer.OnHtmlContentDidChange(id, redirect_chain, content);
  }
}

void TabManager::NotifyDidCloseTab(const int32_t id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnDidCloseTab(id);
  }
}

void TabManager::NotifyTabDidStartPlayingMedia(const int32_t id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTabDidStartPlayingMedia(id);
  }
}

void TabManager::NotifyTabDidStopPlayingMedia(const int32_t id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTabDidStopPlayingMedia(id);
  }
}

void TabManager::OnNotifyTabHtmlContentDidChange(
    const int32_t id,
    const std::vector<GURL>& redirect_chain,
    const std::string& content) {
  CHECK(!redirect_chain.empty());

  const uint32_t hash = base::FastHash(content);
  if (hash == last_html_content_hash_) {
    return;
  }
  last_html_content_hash_ = hash;

  BLOG(2, "Tab id " << id << " HTML content changed");

  NotifyHtmlContentDidChange(id, redirect_chain, content);
}

void TabManager::OnNotifyTabTextContentDidChange(
    const int32_t id,
    const std::vector<GURL>& redirect_chain,
    const std::string& content) {
  CHECK(!redirect_chain.empty());

  const uint32_t hash = base::FastHash(content);
  if (hash == last_text_content_hash_) {
    return;
  }
  last_text_content_hash_ = hash;

  BLOG(2, "Tab id " << id << " text content changed");

  NotifyTextContentDidChange(id, redirect_chain, content);
}

void TabManager::OnNotifyTabDidStartPlayingMedia(const int32_t id) {
  if (tabs_[id].is_playing_media) {
    return;
  }

  BLOG(2, "Tab id " << id << " is playing media");

  tabs_[id].is_playing_media = true;

  NotifyTabDidStartPlayingMedia(id);
}

void TabManager::OnNotifyTabDidStopPlayingMedia(const int32_t id) {
  if (!tabs_[id].is_playing_media) {
    return;
  }

  BLOG(2, "Tab id " << id << " stopped playing media");

  tabs_[id].is_playing_media = false;

  NotifyTabDidStopPlayingMedia(id);
}

void TabManager::OnNotifyTabDidChange(const int32_t id,
                                      const std::vector<GURL>& redirect_chain,
                                      const bool is_visible,
                                      const bool is_incognito) {
  if (is_incognito) {
    BLOG(7, "Tab id " << id << " is incognito");
    return;
  }

  if (!is_visible) {
    BLOG(7, "Tab id " << id << " is occluded");

    if (!GetForId(id)) {
      // Re-add reloaded tabs when browser is restarted
      TabInfo tab;
      tab.id = id;
      tab.redirect_chain = redirect_chain;
      Add(tab);
    } else {
      if (tabs_[id].redirect_chain == redirect_chain) {
        return;
      }

      BLOG(2, "Tab id " << id << " did change");

      tabs_[id].redirect_chain = redirect_chain;

      NotifyTabDidChange(tabs_[id]);
    }

    return;
  }

  if (visible_tab_id_ == id) {
    if (tabs_[id].redirect_chain == redirect_chain) {
      return;
    }

    BLOG(2, "Tab id " << id << " was updated");

    tabs_[id].redirect_chain = redirect_chain;

    return NotifyTabDidChange(tabs_[id]);
  }

  BLOG(2, "Tab id " << id << " is visible");

  last_visible_tab_id_ = visible_tab_id_;

  visible_tab_id_ = id;

  if (const absl::optional<TabInfo> tab = GetForId(id)) {
    BLOG(2, "Focused on existing tab id " << id);

    Update(*tab);

    return NotifyTabDidChangeFocus(id);
  }

  BLOG(2, "Opened a new tab with id " << id);

  TabInfo tab;
  tab.id = id;
  tab.redirect_chain = redirect_chain;
  Add(tab);

  NotifyDidOpenNewTab(tab);
}

void TabManager::OnNotifyDidCloseTab(const int32_t id) {
  BLOG(2, "Tab id " << id << " was closed");

  Remove(id);

  NotifyDidCloseTab(id);
}

}  // namespace brave_ads
