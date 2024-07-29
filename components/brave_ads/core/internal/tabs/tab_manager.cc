/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/hash/hash.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "url/gurl.h"

namespace brave_ads {

TabManager::TabManager() {
  AddAdsClientNotifierObserver(this);
}

TabManager::~TabManager() {
  RemoveAdsClientNotifierObserver(this);
}

// static
TabManager& TabManager::GetInstance() {
  return GlobalState::GetInstance()->GetTabManager();
}

void TabManager::AddObserver(TabManagerObserver* const observer) {
  CHECK(observer);

  observers_.AddObserver(observer);
}

void TabManager::RemoveObserver(TabManagerObserver* const observer) {
  CHECK(observer);

  observers_.RemoveObserver(observer);
}

std::optional<TabInfo> TabManager::MaybeGetVisible() const {
  if (!visible_tab_id_) {
    // `OnNotifyTabDidChange` was not invoked for a tab that is currently
    // visible in the browsing session.
    return std::nullopt;
  }

  return MaybeGetForId(*visible_tab_id_);
}

std::optional<TabInfo> TabManager::MaybeGetForId(const int32_t tab_id) const {
  if (!DoesExistForId(tab_id)) {
    return std::nullopt;
  }

  return tabs_.at(tab_id);
}

bool TabManager::IsPlayingMedia(const int32_t tab_id) const {
  const std::optional<TabInfo> tab = MaybeGetForId(tab_id);
  return tab ? tab->is_playing_media : false;
}

///////////////////////////////////////////////////////////////////////////////

bool TabManager::DoesExistForId(const int32_t tab_id) const {
  return base::Contains(tabs_, tab_id);
}

TabInfo& TabManager::GetOrCreateForId(const int32_t tab_id) {
  if (!DoesExistForId(tab_id)) {
    // Create a new tab.
    TabInfo tab;
    tab.id = tab_id;
    tabs_[tab_id] = tab;
  }

  return tabs_[tab_id];
}

void TabManager::RemoveForId(const int32_t tab_id) {
  tabs_.erase(tab_id);

  if (tabs_.empty()) {
    BLOG(2, "There are no tabs");
    visible_tab_id_.reset();
  }
}

void TabManager::NotifyTabDidChangeFocus(const int32_t tab_id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTabDidChangeFocus(tab_id);
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
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTextContentDidChange(tab_id, redirect_chain, text);
  }
}

void TabManager::NotifyHtmlContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  for (TabManagerObserver& observer : observers_) {
    observer.OnHtmlContentDidChange(tab_id, redirect_chain, html);
  }
}

void TabManager::NotifyDidCloseTab(const int32_t tab_id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnDidCloseTab(tab_id);
  }
}

void TabManager::NotifyTabDidStartPlayingMedia(const int32_t tab_id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTabDidStartPlayingMedia(tab_id);
  }
}

void TabManager::NotifyTabDidStopPlayingMedia(const int32_t tab_id) const {
  for (TabManagerObserver& observer : observers_) {
    observer.OnTabDidStopPlayingMedia(tab_id);
  }
}

void TabManager::OnNotifyTabHtmlContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  CHECK(!redirect_chain.empty());

  const uint32_t hash = base::FastHash(html);
  if (!html.empty() && hash == last_html_content_hash_) {
    // No change.
    return;
  }
  last_html_content_hash_ = hash;

  BLOG(2, "Tab id " << tab_id << " HTML content changed");
  NotifyHtmlContentDidChange(tab_id, redirect_chain, html);
}

void TabManager::OnNotifyTabTextContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  CHECK(!redirect_chain.empty());

  const uint32_t hash = base::FastHash(text);
  if (hash == last_text_content_hash_) {
    // No change.
    return;
  }
  last_text_content_hash_ = hash;

  BLOG(2, "Tab id " << tab_id << " text content changed");
  NotifyTextContentDidChange(tab_id, redirect_chain, text);
}

void TabManager::OnNotifyTabDidStartPlayingMedia(const int32_t tab_id) {
  TabInfo& tab = GetOrCreateForId(tab_id);
  if (tab.is_playing_media) {
    // Already playing media.
    return;
  }
  tab.is_playing_media = true;

  BLOG(2, "Tab id " << tab_id << " started playing media");
  NotifyTabDidStartPlayingMedia(tab_id);
}

void TabManager::OnNotifyTabDidStopPlayingMedia(const int32_t tab_id) {
  TabInfo& tab = GetOrCreateForId(tab_id);
  if (!tab.is_playing_media) {
    // Not playing media.
    return;
  }
  tab.is_playing_media = false;

  BLOG(2, "Tab id " << tab_id << " stopped playing media");
  NotifyTabDidStopPlayingMedia(tab_id);
}

void TabManager::OnNotifyTabDidChange(const int32_t tab_id,
                                      const std::vector<GURL>& redirect_chain,
                                      const bool is_new_navigation,
                                      const bool is_restoring,
                                      const bool is_error_page,
                                      const bool is_visible) {
  CHECK(!redirect_chain.empty());

  const bool does_exist = DoesExistForId(tab_id);

  TabInfo& tab = GetOrCreateForId(tab_id);

  // Check if the tab changed.
  const bool did_change =
      does_exist && is_new_navigation && tab.redirect_chain != redirect_chain;

  // Check if the tab changed focus.
  const bool did_change_focus = !does_exist || tab.is_visible != is_visible;

  // Update the tab.
  tab.is_visible = is_visible;
  tab.redirect_chain = redirect_chain;
  tab.is_error_page = is_error_page;

  if (is_visible) {
    // Update the visible tab id.
    visible_tab_id_ = tab_id;
  }

  if (is_restoring) {
    return BLOG(2, "Restored " << (is_visible ? "focused" : "occluded")
                               << " tab with id " << tab_id);
  }

  // Notify observers.
  if (!does_exist) {
    BLOG(2, "Created tab with id " << tab_id);
    NotifyDidOpenNewTab(tab);
  }

  if (did_change) {
    BLOG(2, "Tab id " << tab_id << " did change");
    NotifyTabDidChange(tab);
  }

  if (did_change_focus) {
    BLOG(2, "Tab id " << tab_id << " did become "
                      << (is_visible ? "focused" : "occluded"));
    NotifyTabDidChangeFocus(tab_id);
  }
}

void TabManager::OnNotifyDidCloseTab(const int32_t tab_id) {
  BLOG(2, "Tab id " << tab_id << " was closed");

  RemoveForId(tab_id);

  NotifyDidCloseTab(tab_id);
}

}  // namespace brave_ads
