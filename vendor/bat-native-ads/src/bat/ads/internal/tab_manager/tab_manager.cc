/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tab_manager/tab_manager.h"

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/tab_manager/tab_info.h"
#include "bat/ads/internal/user_activity/user_activity.h"

namespace ads {

namespace {
TabManager* g_tab_manager = nullptr;
}  // namespace

TabManager::TabManager() {
  DCHECK_EQ(g_tab_manager, nullptr);
  g_tab_manager = this;
}

TabManager::~TabManager() {
  DCHECK(g_tab_manager);
  g_tab_manager = nullptr;
}

// static
TabManager* TabManager::Get() {
  DCHECK(g_tab_manager);
  return g_tab_manager;
}

// static
bool TabManager::HasInstance() {
  return g_tab_manager;
}

bool TabManager::IsVisible(const int32_t id) const {
  if (id == 0) {
    return false;
  }

  return visible_tab_id_ == id;
}

void TabManager::OnUpdated(const int32_t id,
                           const std::string& url,
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
    }

    return;
  }

  if (visible_tab_id_ == id) {
    if (tabs_[id].url == url) {
      return;
    }

    BLOG(2, "Tab id " << id << " was updated");

    UserActivity::Get()->RecordEvent(UserActivityEventType::kTabUpdated);

    tabs_[id].url = url;

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

    UserActivity::Get()->RecordEvent(
        UserActivityEventType::kFocusedOnExistingTab);

    UpdateTab(id, tab);
  } else {
    BLOG(2, "Opened a new tab with id " << id);

    UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);

    AddTab(id, tab);
  }
}

void TabManager::OnClosed(const int32_t id) {
  BLOG(2, "Tab id " << id << " was closed");

  RemoveTab(id);

  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);
}

void TabManager::OnMediaPlaying(const int32_t id) {
  if (tabs_[id].is_playing_media) {
    return;
  }

  BLOG(2, "Tab id " << id << " is playing media");

  UserActivity::Get()->RecordEvent(UserActivityEventType::kPlayedMedia);

  tabs_[id].is_playing_media = true;
}

void TabManager::OnMediaStopped(const int32_t id) {
  if (!tabs_[id].is_playing_media) {
    return;
  }

  BLOG(2, "Tab id " << id << " stopped playing media");

  UserActivity::Get()->RecordEvent(UserActivityEventType::kStoppedPlayingMedia);

  tabs_[id].is_playing_media = false;
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

}  // namespace ads
