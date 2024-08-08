/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_PLAYLIST_ACTIVE_TAB_TRACKER_H_
#define BRAVE_BROWSER_UI_WEBUI_PLAYLIST_ACTIVE_TAB_TRACKER_H_

#include "base/scoped_observation.h"
#include "brave/components/playlist/browser/playlist_tab_helper_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

namespace content {
class WebContents;
}  // namespace content

namespace playlist {
class PlaylistTabHelper;

// This class helps PlaylistUI to track the active tab and its media state.
class PlaylistActiveTabTracker : public TabStripModelObserver,
                                 public playlist::PlaylistTabHelperObserver {
 public:
  using Callback =
      base::RepeatingCallback<void(bool should_show_add_media_from_page_ui)>;

  PlaylistActiveTabTracker(content::WebContents* playlist_contents,
                           Callback callback);
  PlaylistActiveTabTracker(const PlaylistActiveTabTracker&) = delete;
  PlaylistActiveTabTracker& operator=(const PlaylistActiveTabTracker&) = delete;
  ~PlaylistActiveTabTracker() override;

  bool ShouldShowAddMediaFromPageUI();

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // playlist::PlaylistTabHelperObserver:
  void PlaylistTabHelperWillBeDestroyed() override;
  void OnSavedItemsChanged(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) override;
  void OnFoundItemsChanged(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) override;

 private:
  void OnActiveTabChanged();
  playlist::PlaylistTabHelper* GetPlaylistTabHelperForActiveWebContents();

  raw_ptr<content::WebContents> playlist_contents_ = nullptr;
  Callback callback_;

  base::ScopedObservation<playlist::PlaylistTabHelper,
                          playlist::PlaylistTabHelperObserver>
      playlist_tab_helper_observation_{this};
};

}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_WEBUI_PLAYLIST_ACTIVE_TAB_TRACKER_H_
