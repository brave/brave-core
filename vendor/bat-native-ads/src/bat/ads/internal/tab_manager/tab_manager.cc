/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tab_manager/tab_manager.h"

#include "base/check_op.h"
#include "bat/ads/internal/base/logging_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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
TabManager* TabManager::Get() {
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

bool TabManager::IsVisible(const int32_t id) const {
  if (id == 0) {
    return false;
  }

  return visible_tab_id_ == id;
}

void TabManager::OnUpdated(const int32_t id,
                           const GURL& url,
                           const bool is_visible,
                           const bool is_incognito) {
  if (is_incognito) {
    BLOG(7, "Tab id " << id << " is incognito");
    return;
  }

  if (!is_visible) {
    BLOG(7, "Tab id " << id << " is occluded");

    if (!GetForId(id)) {
      // Readd reloaded tabs when browser is restarted
      TabInfo tab;
      tab.id = id;
      tab.url = url;

      AddTab(id, tab);
    } else {
      if (tabs_[id].url == url) {
        return;
      }

      BLOG(2, "Tab id " << id << " was updated");

      tabs_[id].url = url;

      NotifyTabDidChange(id);
    }

    return;
  }

  if (visible_tab_id_ == id) {
    if (tabs_[id].url == url) {
      return;
    }

    BLOG(2, "Tab id " << id << " was updated");

    tabs_[id].url = url;

    NotifyTabDidChange(id);

    return;
  }

  BLOG(2, "Tab id " << id << " is visible");

  last_visible_tab_id_ = visible_tab_id_;

  visible_tab_id_ = id;

  TabInfo tab;
  tab.id = id;
  tab.url = url;

  if (GetForId(id)) {
    BLOG(2, "Focused on existing tab id " << id);

    UpdateTab(id, tab);

    NotifyTabDidChangeFocus(id);
  } else {
    BLOG(2, "Opened a new tab with id " << id);

    AddTab(id, tab);

    NotifyDidOpenNewTab(id);
  }
}

void TabManager::OnClosed(const int32_t id) {
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
  if (tabs_.find(id) == tabs_.end()) {
    return absl::nullopt;
  }

  return tabs_.at(id);
}

///////////////////////////////////////////////////////////////////////////////

void TabManager::AddTab(const int32_t id, const TabInfo& tab) {
  DCHECK(!GetForId(id));
  tabs_[id] = tab;
}

void TabManager::UpdateTab(const int32_t id, const TabInfo& tab) {
  DCHECK(GetForId(id));
  tabs_[id] = tab;
}

void TabManager::RemoveTab(const int32_t id) {
  tabs_.erase(id);
}

void TabManager::NotifyTabDidChangeFocus(const int32_t id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTabDidChangeFocus(id);
  }
}

void TabManager::NotifyTabDidChange(const int32_t id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTabDidChange(id);
  }
}

void TabManager::NotifyDidOpenNewTab(const int32_t id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnDidOpenNewTab(id);
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
