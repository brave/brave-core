/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/hash/hash.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_helper.h"
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

bool TabManager::IsVisible(const int32_t tab_id) const {
  if (!visible_tab_id_) {
    return false;
  }

  return visible_tab_id_ == tab_id;
}

bool TabManager::IsPlayingMedia(const int32_t tab_id) const {
  const absl::optional<TabInfo> tab = MaybeGetForId(tab_id);
  return tab ? tab->is_playing_media : false;
}

absl::optional<TabInfo> TabManager::GetVisible() const {
  if (!visible_tab_id_) {
    return absl::nullopt;
  }

  return MaybeGetForId(*visible_tab_id_);
}

absl::optional<TabInfo> TabManager::GetLastVisible() const {
  if (!last_visible_tab_id_) {
    return absl::nullopt;
  }

  return MaybeGetForId(*last_visible_tab_id_);
}

absl::optional<TabInfo> TabManager::MaybeGetForId(const int32_t tab_id) const {
  if (!base::Contains(tabs_, tab_id)) {
    return absl::nullopt;
  }

  return tabs_.at(tab_id);
}

///////////////////////////////////////////////////////////////////////////////

TabInfo& TabManager::GetOrCreateForId(const int32_t tab_id) {
  if (!base::Contains(tabs_, tab_id)) {
    TabInfo tab;
    tab.id = tab_id;
    tabs_[tab_id] = tab;
  }

  return tabs_[tab_id];
}

void TabManager::Remove(const int32_t tab_id) {
  tabs_.erase(tab_id);
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
  if (hash != last_html_content_hash_) {
    last_html_content_hash_ = hash;

    BLOG(2, "Tab id " << tab_id << " HTML content changed");
    NotifyHtmlContentDidChange(tab_id, redirect_chain, html);
  }
}

void TabManager::OnNotifyTabTextContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  CHECK(!redirect_chain.empty());

  const uint32_t hash = base::FastHash(text);
  if (hash != last_text_content_hash_) {
    last_text_content_hash_ = hash;

    BLOG(2, "Tab id " << tab_id << " text content changed");
    NotifyTextContentDidChange(tab_id, redirect_chain, text);
  }
}

void TabManager::OnNotifyTabDidStartPlayingMedia(const int32_t tab_id) {
  TabInfo& tab = GetOrCreateForId(tab_id);
  if (!tab.is_playing_media) {
    tab.is_playing_media = true;

    BLOG(2, "Tab id " << tab_id << " started playing media");
    NotifyTabDidStartPlayingMedia(tab_id);
  }
}

void TabManager::OnNotifyTabDidStopPlayingMedia(const int32_t tab_id) {
  TabInfo& tab = GetOrCreateForId(tab_id);
  if (tab.is_playing_media) {
    tab.is_playing_media = false;

    BLOG(2, "Tab id " << tab_id << " stopped playing media");
    NotifyTabDidStopPlayingMedia(tab_id);
  }
}

void TabManager::OnNotifyTabDidChange(const int32_t tab_id,
                                      const std::vector<GURL>& redirect_chain,
                                      const bool is_visible) {
  const bool is_existing_tab = !!MaybeGetForId(tab_id);

  TabInfo& tab = GetOrCreateForId(tab_id);

  const bool redirect_chain_did_change = tab.redirect_chain != redirect_chain;
  if (redirect_chain_did_change) {
    tab.redirect_chain = redirect_chain;
  }

  if (!is_visible) {
    BLOG(7, "Tab id " << tab_id << " is occluded");
    if (!redirect_chain_did_change) {
      return;
    }

    BLOG(2, "Tab id " << tab_id << " did change");
    return NotifyTabDidChange(tab);
  }

  if (visible_tab_id_ == tab_id) {
    BLOG(2, "Tab id " << tab_id << " did change");
    return NotifyTabDidChange(tab);
  }

  BLOG(2, "Tab id " << tab_id << " is visible");

  last_visible_tab_id_ = visible_tab_id_;

  visible_tab_id_ = tab_id;

  if (is_existing_tab) {
    BLOG(2, "Focused on existing tab with id " << tab_id);
    return NotifyTabDidChangeFocus(tab_id);
  }

  BLOG(2, "Opened a new tab with id " << tab_id);
  NotifyDidOpenNewTab(tab);
}

void TabManager::OnNotifyDidCloseTab(const int32_t tab_id) {
  BLOG(2, "Tab id " << tab_id << " was closed");

  Remove(tab_id);

  NotifyDidCloseTab(tab_id);
}

}  // namespace brave_ads
