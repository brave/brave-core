/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/playlist_active_tab_tracker.h"

#include "brave/browser/ui/playlist/playlist_browser_finder.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "chrome/browser/ui/browser.h"

namespace playlist {

PlaylistActiveTabTracker::PlaylistActiveTabTracker(
    content::WebContents* playlist_contents,
    Callback callback)
    : playlist_contents_(playlist_contents), callback_(callback) {
  auto* browser = playlist::FindBrowserForPlaylistWebUI(playlist_contents_);
  CHECK(browser);
  browser->tab_strip_model()->AddObserver(this);
  OnActiveTabChanged();
}

PlaylistActiveTabTracker::~PlaylistActiveTabTracker() = default;

void PlaylistActiveTabTracker::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (!selection.active_tab_changed()) {
    return;
  }

  OnActiveTabChanged();
}

void PlaylistActiveTabTracker::PlaylistTabHelperWillBeDestroyed() {
  playlist_tab_helper_observation_.Reset();
}

void PlaylistActiveTabTracker::OnSavedItemsChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>& items) {
  callback_.Run(ShouldShowAddMediaFromPageUI());
}

void PlaylistActiveTabTracker::OnFoundItemsChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>& items) {
  callback_.Run(ShouldShowAddMediaFromPageUI());
}

void PlaylistActiveTabTracker::OnActiveTabChanged() {
  playlist_tab_helper_observation_.Reset();

  auto* tab_helper = GetPlaylistTabHelperForActiveWebContents();
  if (!tab_helper) {
    return;
  }

  playlist_tab_helper_observation_.Observe(tab_helper);
  callback_.Run(ShouldShowAddMediaFromPageUI());
}

playlist::PlaylistTabHelper*
PlaylistActiveTabTracker::GetPlaylistTabHelperForActiveWebContents() {
  auto* browser = playlist::FindBrowserForPlaylistWebUI(playlist_contents_);
  if (!browser) {
    return nullptr;
  }
  auto* active_web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  if (!active_web_contents) {
    // Can happen on shutdown.
    return nullptr;
  }

  return playlist::PlaylistTabHelper::FromWebContents(active_web_contents);
}

bool PlaylistActiveTabTracker::ShouldShowAddMediaFromPageUI() {
  auto* tab_helper = GetPlaylistTabHelperForActiveWebContents();
  return tab_helper && !tab_helper->found_items().empty() &&
         tab_helper->saved_items().empty();
}

}  // namespace playlist
