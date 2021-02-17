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

  is_foregrounded_ = AdsClientHelper::Get()->IsForeground();
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

bool TabManager::IsForegrounded() const {
  return is_foregrounded_;
}

void TabManager::OnForegrounded() {
  BLOG(1, "Browser window did become active");

  is_foregrounded_ = true;

  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kBrowserWindowDidBecomeActive);
}

void TabManager::OnBackgrounded() {
  BLOG(1, "Browser window did enter background");

  is_foregrounded_ = false;

  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kBrowserWindowDidEnterBackground);
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
    return;
  }

  if (visible_tab_id_ == id) {
    if (tabs_[id].url == url) {
      return;
    }

    BLOG(2, "Tab id " << id << " was updated");

    tabs_[id].url = url;

    return;
  }

  BLOG(2, "Tab id " << id << " is visible");

  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);

  last_visible_tab_id_ = visible_tab_id_;

  visible_tab_id_ = id;

  TabInfo tab;
  tab.id = id;
  tab.url = url;

  tabs_.insert({id, tab});
}

void TabManager::OnClosed(const int32_t id) {
  BLOG(2, "Tab id " << id << " was closed");

  tabs_.erase(id);

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

  tabs_[id].is_playing_media = false;
}

bool TabManager::IsPlayingMedia(const int32_t id) const {
  TabInfo tab;

  if (tabs_.find(id) != tabs_.end()) {
    tab = tabs_.at(id);
  }

  return tab.is_playing_media;
}

base::Optional<TabInfo> TabManager::GetVisible() const {
  return GetForId(visible_tab_id_);
}

base::Optional<TabInfo> TabManager::GetLastVisible() const {
  return GetForId(last_visible_tab_id_);
}

base::Optional<TabInfo> TabManager::GetForId(const int32_t id) const {
  if (tabs_.find(id) == tabs_.end()) {
    return base::nullopt;
  }

  return tabs_.at(id);
}

}  // namespace ads
