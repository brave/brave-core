/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tabs/tab_manager.h"

#include "base/check_op.h"
#include "base/hash/hash.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/tabs/tab_info.h"
#include "url/gurl.h"

namespace ads {

namespace {
TabManager* g_tab_manager_instance = nullptr;
}  // namespace

TabManager::TabManager() {
  DCHECK(!g_tab_manager_instance);
  g_tab_manager_instance = this;
}

TabManager::~TabManager() {
  DCHECK_EQ(this, g_tab_manager_instance);
  g_tab_manager_instance = nullptr;
}

// static
TabManager* TabManager::GetInstance() {
  DCHECK(g_tab_manager_instance);
  return g_tab_manager_instance;
}

// static
bool TabManager::HasInstance() {
  return !!g_tab_manager_instance;
}

void TabManager::AddObserver(TabManagerObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void TabManager::RemoveObserver(TabManagerObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

bool TabManager::IsTabVisible(const int32_t id) const {
  if (id == 0) {
    return false;
  }

  return visible_tab_id_ == id;
}

void TabManager::OnTabUpdated(const int32_t id,
                              const GURL& url,
                              const bool is_visible,
                              const bool is_incognito) {
  if (is_incognito) {
    BLOG(7, "Tab id " << id << " is incognito");
    return;
  }

  if (!is_visible) {
    BLOG(7, "Tab id " << id << " is occluded");

    if (!GetTabForId(id)) {
      // Re-add reloaded tabs when browser is restarted
      TabInfo tab;
      tab.id = id;
      tab.url = url;
      AddTab(tab);
    } else {
      if (tabs_[id].url == url) {
        return;
      }

      BLOG(2, "Tab id " << id << " was updated");

      tabs_[id].url = url;

      NotifyTabDidChange(tabs_[id]);
    }

    return;
  }

  if (visible_tab_id_ == id) {
    if (tabs_[id].url == url) {
      return;
    }

    BLOG(2, "Tab id " << id << " was updated");

    tabs_[id].url = url;

    NotifyTabDidChange(tabs_[id]);

    return;
  }

  BLOG(2, "Tab id " << id << " is visible");

  last_visible_tab_id_ = visible_tab_id_;

  visible_tab_id_ = id;

  if (const auto tab = GetTabForId(id)) {
    BLOG(2, "Focused on existing tab id " << id);

    UpdateTab(*tab);
    NotifyTabDidChangeFocus(id);
    return;
  }

  BLOG(2, "Opened a new tab with id " << id);

  TabInfo tab;
  tab.id = id;
  tab.url = url;
  AddTab(tab);

  NotifyDidOpenNewTab(tab);
}

void TabManager::OnTextContentDidChange(const int32_t id,
                                        const std::vector<GURL>& redirect_chain,
                                        const std::string& content) {
  DCHECK(!redirect_chain.empty());

  const uint32_t hash = base::FastHash(content);
  if (hash == last_text_content_hash_) {
    return;
  }
  last_text_content_hash_ = hash;

  BLOG(2, "Tab id " << id << " text content changed");

  NotifyTextContentDidChange(id, redirect_chain, content);
}

void TabManager::OnHtmlContentDidChange(const int32_t id,
                                        const std::vector<GURL>& redirect_chain,
                                        const std::string& content) {
  DCHECK(!redirect_chain.empty());

  const uint32_t hash = base::FastHash(content);
  if (hash == last_html_content_hash_) {
    return;
  }
  last_html_content_hash_ = hash;

  BLOG(2, "Tab id " << id << " HTML content changed");

  NotifyHtmlContentDidChange(id, redirect_chain, content);
}

void TabManager::OnTabClosed(const int32_t id) {
  BLOG(2, "Tab id " << id << " was closed");

  RemoveTab(id);

  NotifyDidCloseTab(id);
}

void TabManager::OnMediaPlaying(const int32_t id) {
  if (tabs_[id].is_playing_media) {
    return;
  }

  BLOG(2, "Tab id " << id << " is playing media");

  tabs_[id].is_playing_media = true;

  NotifyTabDidStartPlayingMedia(id);
}

void TabManager::OnMediaStopped(const int32_t id) {
  if (!tabs_[id].is_playing_media) {
    return;
  }

  BLOG(2, "Tab id " << id << " stopped playing media");

  tabs_[id].is_playing_media = false;

  NotifyTabDidStopPlayingMedia(id);
}

bool TabManager::IsPlayingMedia(const int32_t id) const {
  const absl::optional<TabInfo> tab = GetTabForId(id);
  if (!tab) {
    return false;
  }

  return tab->is_playing_media;
}

absl::optional<TabInfo> TabManager::GetVisibleTab() const {
  return GetTabForId(visible_tab_id_);
}

absl::optional<TabInfo> TabManager::GetLastVisibleTab() const {
  return GetTabForId(last_visible_tab_id_);
}

absl::optional<TabInfo> TabManager::GetTabForId(const int32_t id) const {
  if (tabs_.find(id) == tabs_.end()) {
    return absl::nullopt;
  }

  return tabs_.at(id);
}

///////////////////////////////////////////////////////////////////////////////

void TabManager::AddTab(const TabInfo& tab) {
  DCHECK(!GetTabForId(tab.id));
  tabs_[tab.id] = tab;
}

void TabManager::UpdateTab(const TabInfo& tab) {
  DCHECK(GetTabForId(tab.id));
  tabs_[tab.id] = tab;
}

void TabManager::RemoveTab(const int32_t id) {
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

}  // namespace ads
