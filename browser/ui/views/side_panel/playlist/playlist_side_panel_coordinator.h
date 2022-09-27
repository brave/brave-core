/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_PLAYLIST_PLAYLIST_SIDE_PANEL_COORDINATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_PLAYLIST_PLAYLIST_SIDE_PANEL_COORDINATOR_H_

#include <memory>

#include "chrome/browser/ui/browser_user_data.h"
#include "chrome/browser/ui/views/bubble/bubble_contents_wrapper.h"

namespace playlist {
class PlaylistUI;
}  // namespace playlist

namespace views {
class View;
}  // namespace views

class Browser;
class SidePanelRegistry;

class PlaylistSidePanelCoordinator
    : public BrowserUserData<PlaylistSidePanelCoordinator> {
 public:
  explicit PlaylistSidePanelCoordinator(Browser* browser);
  PlaylistSidePanelCoordinator(const PlaylistSidePanelCoordinator&) = delete;
  PlaylistSidePanelCoordinator& operator=(const PlaylistSidePanelCoordinator&) =
      delete;
  ~PlaylistSidePanelCoordinator() override;

  void CreateAndRegisterEntry(SidePanelRegistry* global_registry);

 private:
  friend class BrowserUserData<PlaylistSidePanelCoordinator>;

  std::unique_ptr<views::View> CreateWebView();

  std::unique_ptr<BubbleContentsWrapperT<playlist::PlaylistUI>>
      contents_wrapper_;

  BROWSER_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_PLAYLIST_PLAYLIST_SIDE_PANEL_COORDINATOR_H_
