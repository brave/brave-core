/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_PLAYLIST_PLAYLIST_SIDE_PANEL_WEB_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_PLAYLIST_PLAYLIST_SIDE_PANEL_WEB_VIEW_H_

class Browser;

#include "base/callback_forward.h"
#include "brave/browser/ui/webui/playlist_ui.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"

class PlaylistSidePanelWebView
    : public SidePanelWebUIViewT<playlist::PlaylistUI> {
 public:
  PlaylistSidePanelWebView(Browser* browser, base::RepeatingClosure close_cb);
  PlaylistSidePanelWebView(const PlaylistSidePanelWebView&) = delete;
  PlaylistSidePanelWebView& operator=(const PlaylistSidePanelWebView&) = delete;
  ~PlaylistSidePanelWebView() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_PLAYLIST_PLAYLIST_SIDE_PANEL_WEB_VIEW_H_
